#include "DistJob.h"

#include <cstdio>
#include <format>
#include <functional>
#include <map>
#include <unordered_map>
#include <utility>

#include <windows.h>

#include "DistHashCache.h"
#include "DistManifestBuilder.h"
#include "DistReferences.h"
#include "DistScan.h"
#include "DistSelfTest.h"
#include "DistUpload.h"
#include "Rclone.h"
#include "../Text/Utf8.h"
#include "../Text/VersionString.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr wchar_t DIST_JOB_ASSETS_ROOT[]   = L"Assets";
        constexpr wchar_t DIST_JOB_MANIFEST_FILE[] = L"manifest.json";
        constexpr wchar_t DIST_JOB_CACHE_FILE[]    = L".manifest_hash_cache.json";
        constexpr size_t  DIST_JOB_SAMPLE_LIMIT    = 10;

        // NOTE: 一時フォルダ。抜けるときに中身ごと消す
        class DistJobStagingDirectory final
        {
        public:
            DistJobStagingDirectory()
            {
                std::error_code error;
                const std::filesystem::path base = std::filesystem::temp_directory_path(error);
                if (error)
                    return;
                for (int attempt = 0; attempt < 16; ++attempt)
                {
                    const std::filesystem::path candidate = base / std::format(L"nanami-dist-{}-{}-{}", GetCurrentProcessId(), GetTickCount64(), attempt);
                    if (std::filesystem::create_directory(candidate, error) && !error)
                    {
                        path_ = candidate;
                        return;
                    }
                }
            }
            ~DistJobStagingDirectory()
            {
                if (path_.empty())
                    return;
                std::error_code error;
                std::filesystem::remove_all(path_, error);
            }
            DistJobStagingDirectory(const DistJobStagingDirectory&)            = delete;
            DistJobStagingDirectory& operator=(const DistJobStagingDirectory&) = delete;

            [[nodiscard]] const std::filesystem::path& Path() const { return path_; }

        private:
            std::filesystem::path path_;
        };

        class DistJobScopeExit final
        {
        public:
            explicit DistJobScopeExit(std::function<void()> onExit) : onExit_(std::move(onExit)) {}
            ~DistJobScopeExit() { onExit_(); }
            DistJobScopeExit(const DistJobScopeExit&)            = delete;
            DistJobScopeExit& operator=(const DistJobScopeExit&) = delete;

        private:
            std::function<void()> onExit_;
        };
    }

    DistJob::~DistJob()
    {
        Cancel();
        Join();
    }

    bool DistJob::Start(DistRequest request)
    {
        if (running_)
            return false;
        Join();

        {
            std::lock_guard lock(mutex_);
            lines_.clear();
            exitCode_.reset();
            startTime_ = std::chrono::steady_clock::now();
            endTime_   = startTime_;
        }
        canceled_ = false;
        running_  = true;
        worker_   = std::thread([this, request = std::move(request)] { Run(request); });
        return true;
    }

    void DistJob::Cancel()
    {
        if (!running_)
            return;
        canceled_ = true;
        std::lock_guard lock(mutex_);
        if (rclone_)
            rclone_->Cancel();
    }

    bool DistJob::IsRunning() const
    {
        return running_;
    }

    bool DistJob::WasCanceled() const
    {
        return canceled_;
    }

    std::optional<int> DistJob::ExitCode() const
    {
        std::lock_guard lock(mutex_);
        return exitCode_;
    }

    std::string DistJob::ElapsedLabel() const
    {
        std::lock_guard lock(mutex_);
        const auto end     = running_ ? std::chrono::steady_clock::now() : endTime_;
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - startTime_).count();
        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%d:%02d", static_cast<int>(seconds / 60), static_cast<int>(seconds % 60));
        return buffer;
    }

    size_t DistJob::CopyLines(std::vector<std::string>& out, const size_t from) const
    {
        std::lock_guard lock(mutex_);
        for (size_t i = from; i < lines_.size(); ++i)
            out.push_back(lines_[i]);
        return lines_.size();
    }

    void DistJob::Print(std::string line)
    {
        std::lock_guard lock(mutex_);
        lines_.push_back(std::move(line));
    }

    void DistJob::SetActiveRclone(Rclone* const rclone)
    {
        std::lock_guard lock(mutex_);
        rclone_ = rclone;
        if (rclone_ && canceled_)
            rclone_->Cancel();
    }

    bool DistJob::IsCanceled() const
    {
        return canceled_;
    }

    void DistJob::Join()
    {
        if (worker_.joinable())
            worker_.join();
    }

    void DistJob::Run(const DistRequest& request)
    {
        int exitCode = 1;
        try
        {
            switch (request.step)
            {
            case DistStep::Build:    exitCode = RunBuild(request);          break;
            case DistStep::DryRun:   exitCode = RunUpload(request, true);   break;
            case DistStep::Release:  exitCode = RunUpload(request, false);  break;
            case DistStep::DiffLive: exitCode = RunDiffLive(request);       break;
            case DistStep::SelfTest: exitCode = RunDistSelfTest([this](std::string line) { Print(std::move(line)); }, request.repoRoot); break;
            case DistStep::None:     break;
            }
        }
        catch (const DistCanceledError&)
        {
        }
        catch (const std::exception& exception)
        {
            Print(std::string("ERROR: ") + exception.what());
            exitCode = 1;
        }

        {
            std::lock_guard lock(mutex_);
            if (IsCanceled())
                lines_.emplace_back("中止しました");
            else
                exitCode_ = exitCode;
            endTime_ = std::chrono::steady_clock::now();
        }
        running_ = false;
    }

    int DistJob::RunBuild(const DistRequest& request)
    {
        const std::filesystem::path& repoRoot   = request.repoRoot;
        const std::filesystem::path  assetsRoot = repoRoot / DIST_JOB_ASSETS_ROOT;
        const std::filesystem::path  outPath    = repoRoot / DIST_JOB_MANIFEST_FILE;
        const auto isCanceled = [this] { return IsCanceled(); };

        if (!IsValidVersionString(request.version))
        {
            Print(std::format("ERROR: version '{}' は版として使えません", request.version));
            return 1;
        }
        if (!IsValidVersionString(request.requiredClientVersion))
        {
            Print(std::format("ERROR: requiredClientVersion '{}' は版として使えません", request.requiredClientVersion));
            return 1;
        }

        DistHashCache cache(repoRoot / DIST_JOB_CACHE_FILE);
        Print("Assets/ を走査しています...");
        const DistScanResult result = ScanAssets(assetsRoot, repoRoot, cache, isCanceled);
        if (result.canceled || IsCanceled())
        {
            // NOTE: 途中までのハッシュも次回に使える
            cache.Save();
            return 1;
        }
        if (!result.error.empty())
        {
            cache.Save();
            Print("ERROR: " + result.error);
            return 1;
        }
        const DistRefReport refReport = FindUnshippedRefs(result, repoRoot, assetsRoot, cache, isCanceled);
        cache.Save();
        if (refReport.canceled)
            return 1;

        if (!refReport.unshipped.empty())
        {
            std::map<std::string, std::vector<const DistUnshippedRef*>> byOwner;
            std::vector<std::string> owners;
            for (const DistUnshippedRef& item : refReport.unshipped)
            {
                if (!byOwner.contains(item.path))
                    owners.push_back(item.path);
                byOwner[item.path].push_back(&item);
            }
            Print(std::format("ERROR: 配信されないファイルを参照している .efkefc / .mv1 が {} 件あります。", owners.size()));
            Print("       この PC では表示されるが、プレイヤーの PC ではテクスチャ / モデルが見つからない。");
            Print("       参照先を Assets/ 内の配信されるコピーに張り替えること (.efkefc は .efkproj と同じフォルダへ");
            Print("       コンパイルし直す)。manifest は書き出していません");
            for (const std::string& owner : owners)
            {
                Print("  " + owner);
                for (const DistUnshippedRef* item : byOwner[owner])
                    Print(std::format("    {}  ({})", item->ref, item->reason));
            }
            return 1;
        }

        const std::string   baseUrl  = request.config.FilesBaseUrl();
        const AssetManifest manifest = BuildManifest(result, request.version, request.requiredClientVersion, baseUrl);
        if (const std::string error = WriteManifestFile(manifest, outPath); !error.empty())
        {
            Print("ERROR: " + error);
            return 1;
        }

        Print("scanned  Assets");
        Print(std::format("  assets      {:>5}  (.meta あり)", result.AssetCount()));
        Print(std::format("  companions  {:>5}  (.meta 無し / 相対参照される随伴ファイル)", result.CompanionCount()));
        Print(std::format("  .meta       {:>5}  (本体エントリに畳んだ)", result.foldedMeta.size()));
        Print(std::format("  excluded    {:>5}  (開発専用)", result.excluded.size()));
        Print(std::format("  hashed      {:>5}  (cache hit {})", cache.misses, cache.hits));
        Print(std::format("  refs read   {:>5}  (cache hit {}, .efkefc / .mv1 の参照先)", cache.refMisses, cache.refHits));
        Print("");
        Print(std::format("manifest {} -> {}", request.version, WideToUtf8(outPath.wstring())));
        Print(std::format("  requiredClientVersion {}", request.requiredClientVersion));
        Print(std::format("  entries     {:>5}", result.entries.size()));
        Print(std::format("  total       {:>9}", FormatBytes(result.TotalBytes())));
        Print(std::format("  baseUrl     {}", baseUrl.empty() ? std::string("(未設定: upload できません)") : baseUrl));

        const auto printSamples = [this](const std::vector<std::string>& paths)
        {
            for (size_t i = 0; i < paths.size() && i < DIST_JOB_SAMPLE_LIMIT; ++i)
                Print("  " + paths[i]);
        };
        if (!result.missingGuid.empty())
        {
            Print("");
            Print(std::format("WARNING: .meta はあるが guid を読めなかったファイルが {} 件あります", result.missingGuid.size()));
            printSamples(result.missingGuid);
        }
        if (!refReport.unreadable.empty())
        {
            Print("");
            Print(std::format("WARNING: 参照先を読み出せなかった .efkefc / .mv1 が {} 件あります (この PC にしか無いファイルを参照していても検出できません)", refReport.unreadable.size()));
            printSamples(refReport.unreadable);
        }
        if (!result.nonAscii.empty())
        {
            // NOTE: ダウンロード URL は baseUrl + <hash> なので影響しない
            Print("");
            Print(std::format("NOTE: 非ASCII のパスが {} 件あります (クライアントはローカルへ書くとき UTF-8 → UTF-16 変換が必要)", result.nonAscii.size()));
            printSamples(result.nonAscii);
        }
        if (!result.shiftJisMeta.empty())
        {
            Print("");
            Print(std::format("NOTE: CP932 のまま残っている .meta が {} 件あります", result.shiftJisMeta.size()));
            printSamples(result.shiftJisMeta);
        }
        return 0;
    }

    int DistJob::RunUpload(const DistRequest& request, const bool dryRun)
    {
        const DistConfig&           config       = request.config;
        const std::filesystem::path manifestPath = request.repoRoot / DIST_JOB_MANIFEST_FILE;
        if (config.remote.empty())
        {
            Print("ERROR: 配信先 (Remote) がありません。Asset Dist の設定で指定してください");
            return 1;
        }

        AssetManifest document;
        if (std::string error; !AssetManifest::TryLoadFile(manifestPath, document, error))
        {
            Print("ERROR: manifest.json を読めませんでした: " + error);
            return 1;
        }
        const std::string& version = document.version;
        if (!IsValidVersionString(version))
        {
            Print(std::format("ERROR: version '{}' はファイル名に使えません", version));
            return 1;
        }
        if (!document.baseUrl.ends_with("/files/"))
        {
            Print(std::format("ERROR: baseUrl が files/ を指していません: '{}'", document.baseUrl));
            Print("       クライアントは baseUrl + <hash> で取りに来るので、build し直してください");
            return 1;
        }
        if (!config.FilesBaseUrl().empty() && document.baseUrl != config.FilesBaseUrl())
            Print(std::format("WARNING: baseUrl ({}) が設定の公開 URL ({}) と違います", document.baseUrl, config.FilesBaseUrl()));

        Rclone rclone(config.rclone, config.remote, [this](const std::string& line) { Print(line); });
        SetActiveRclone(&rclone);
        const DistJobScopeExit clearRclone([this] { SetActiveRclone(nullptr); });
        if (IsCanceled())
            return 1;

        const std::map<std::string, DistBlob> blobs = BlobsOf(document, request.repoRoot);
        const std::string versioned = "manifest-" + version + ".json";
        std::optional<AssetManifest>    live;
        std::unordered_set<std::string> existing;
        try
        {
            if (rclone.VersionedConflicts(manifestPath, versioned))
            {
                Print(std::format("ERROR: {} はすでに別の内容で存在します。Version を上げて build し直してください", versioned));
                return 1;
            }
            live     = rclone.ReadLiveManifest();
            existing = rclone.ListBlobHashes();
        }
        catch (const DistUploadError& error)
        {
            Print(std::string("ERROR: ") + error.what());
            return 1;
        }

        if (live)
        {
            const std::vector<std::string> locked = LockedFileChanges(*live, document);
            if (!locked.empty() && !IsOlderVersion(live->requiredClientVersion, document.requiredClientVersion))
            {
                Print(std::format("ERROR: 公開中の {} から、ゲームの実行中は差し替えられないファイルが変わります。", live->version));
                Print("       フォントは起動時に Windows へ登録されて終了まで外れないので、タイトル画面の更新では置き換えられず、");
                Print("       全員の適用が失敗し続けます。新しい zip を配ったうえで、Required Client Version を");
                Print(std::format("       公開中の '{}' より上げて build し直してください", live->requiredClientVersion));
                for (size_t i = 0; i < locked.size() && i < DIST_JOB_SAMPLE_LIMIT; ++i)
                    Print("  " + locked[i]);
                return 1;
            }
        }

        const std::vector<DistBlob> todo = PlanUpload(blobs, existing);
        std::uint64_t todoBytes = 0;
        for (const DistBlob& blob : todo)
            todoBytes += blob.size;
        Print(std::format("manifest {}  ({})", version, WideToUtf8(manifestPath.wstring())));
        Print(std::format("remote   {}", config.remote));
        Print(std::format("  blobs referenced   {:>5}", blobs.size()));
        Print(std::format("  already on remote  {:>5}", blobs.size() - todo.size()));
        Print(std::format("  to upload          {:>5}  ({})", todo.size(), FormatBytes(todoBytes)));

        const auto printProblems = [this](const std::vector<std::string>& problems)
        {
            if (problems.empty())
                return;
            Print("");
            Print(std::format("ERROR: 検証に失敗したファイルが {} 件あります", problems.size()));
            for (size_t i = 0; i < problems.size() && i < DIST_JOB_SAMPLE_LIMIT; ++i)
                Print("  " + problems[i]);
        };
        const auto isCanceled = [this] { return IsCanceled(); };

        if (dryRun)
        {
            const std::vector<std::string> problems = StageBlobs(todo, {}, isCanceled);
            if (IsCanceled())
                return 1;
            printProblems(problems);
            Print("");
            Print("(dry-run: 何も上げていません)");
            return problems.empty() ? 0 : 1;
        }

        try
        {
            {
                const DistJobStagingDirectory staging;
                if (staging.Path().empty())
                {
                    Print("ERROR: 一時フォルダを作れませんでした");
                    return 1;
                }
                const std::vector<std::string> problems = StageBlobs(todo, staging.Path(), isCanceled);
                if (IsCanceled())
                    return 1;
                if (!problems.empty())
                {
                    printProblems(problems);
                    Print("");
                    Print("中止しました (何も上げていません)");
                    return 1;
                }
                if (!todo.empty())
                {
                    Print("");
                    rclone.UploadBlobs(staging.Path());
                }
            }

            // NOTE: マニフェストが参照するものが全部そろってからでないと、manifest.json は置かない
            const std::unordered_set<std::string> uploaded = rclone.ListBlobHashes();
            size_t missing = 0;
            for (const auto& [digest, blob] : blobs)
            {
                if (!uploaded.contains(digest))
                    ++missing;
            }
            if (missing > 0)
            {
                Print("");
                Print(std::format("ERROR: 上げたあとも remote に無いブロブが {} 件あります。manifest は置いていません", missing));
                return 1;
            }

            Print("");
            if (rclone.UploadFileOnce(manifestPath, versioned, DIST_BLOB_CACHE_CONTROL))
                Print("  uploaded  " + versioned);
            else
                Print("  unchanged " + versioned + " (同じ内容がすでにある)");

            rclone.UploadFile(manifestPath, "manifest.json", DIST_MANIFEST_CACHE_CONTROL);
            Print("  released  manifest.json -> " + version);
        }
        catch (const DistUploadError& error)
        {
            Print("");
            Print(std::string("ERROR: ") + error.what());
            return 1;
        }

        if (const std::string manifestUrl = config.ManifestUrl(); !manifestUrl.empty())
        {
            Print("");
            Print("  " + manifestUrl);
        }
        return 0;
    }

    int DistJob::RunDiffLive(const DistRequest& request)
    {
        const DistConfig&           config       = request.config;
        const std::filesystem::path manifestPath = request.repoRoot / DIST_JOB_MANIFEST_FILE;
        if (config.remote.empty())
        {
            Print("ERROR: 配信先 (Remote) がありません。Asset Dist の設定で指定してください");
            return 1;
        }

        AssetManifest local;
        if (std::string error; !AssetManifest::TryLoadFile(manifestPath, local, error))
        {
            Print("ERROR: manifest.json を読めませんでした (先に Build Manifest): " + error);
            return 1;
        }

        Rclone rclone(config.rclone, config.remote, [this](const std::string& line) { Print(line); });
        SetActiveRclone(&rclone);
        const DistJobScopeExit clearRclone([this] { SetActiveRclone(nullptr); });
        if (IsCanceled())
            return 1;

        std::optional<AssetManifest> live;
        try
        {
            live = rclone.ReadLiveManifest();
        }
        catch (const DistUploadError& error)
        {
            Print(std::string("ERROR: ") + error.what());
            return 1;
        }
        if (!live)
        {
            Print("公開中の manifest.json はまだありません (初回リリースは全件が新規)");
            live = AssetManifest{};
            live->version = "(none)";
        }

        const ManifestDiff diff = ManifestDiff::Between(*live, local);
        Print(std::format("{} -> {}", live->version, local.version));
        if (diff.IsUpToDate())
        {
            Print("  更新なし");
            return 0;
        }
        Print(std::format("  新規 {} / 変更 {} / 削除 {} / ダウンロード {}", diff.added.size(), diff.changed.size(), diff.removedPaths.size(), FormatBytes(diff.downloadBytes)));

        size_t shown = 0;
        for (const ManifestEntry& entry : diff.added)
        {
            if (shown++ >= DIST_JOB_SAMPLE_LIMIT)
                break;
            Print(std::format("  + {} ({})", entry.path, FormatBytes(entry.TotalSize())));
        }
        for (const ManifestEntry& entry : diff.changed)
        {
            if (shown++ >= DIST_JOB_SAMPLE_LIMIT)
                break;
            Print(std::format("  ~ {} ({})", entry.path, FormatBytes(entry.TotalSize())));
        }
        for (const std::string& path : diff.removedPaths)
        {
            if (shown++ >= DIST_JOB_SAMPLE_LIMIT)
                break;
            Print("  - " + path);
        }
        return 0;
    }
}
