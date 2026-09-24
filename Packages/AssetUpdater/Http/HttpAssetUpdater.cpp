#include "HttpAssetUpdater.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

#include "WinHttpClient.h"
#include "../Hash/Sha256.h"
#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr wchar_t HTTP_ASSET_UPDATER_USER_AGENT[]     = L"NanamiEngine AssetUpdater";
        constexpr int     HTTP_ASSET_UPDATER_ATTEMPTS         = 3;
        constexpr int     HTTP_ASSET_UPDATER_RETRY_WAIT_MSEC  = 500;

        std::vector<unsigned long long> HttpAssetUpdaterSplitVersion(const std::string& version)
        {
            std::vector<unsigned long long> parts;
            unsigned long long value = 0;
            for (const char character : version)
            {
                if (character == '.')
                {
                    parts.push_back(value);
                    value = 0;
                    continue;
                }
                if (character >= '0' && character <= '9')
                    value = value * 10 + static_cast<unsigned long long>(character - '0');
            }
            parts.push_back(value);
            return parts;
        }

        /** "1.9.0" < "1.10.0" を文字列比較で取り違えないよう、ドット区切りの数値として比べる */
        bool HttpAssetUpdaterIsOlderVersion(const std::string& left, const std::string& right)
        {
            const std::vector<unsigned long long> leftParts  = HttpAssetUpdaterSplitVersion(left);
            const std::vector<unsigned long long> rightParts = HttpAssetUpdaterSplitVersion(right);

            const std::size_t count = (std::max)(leftParts.size(), rightParts.size());
            for (std::size_t i = 0; i < count; ++i)
            {
                const unsigned long long leftValue  = i < leftParts.size()  ? leftParts[i]  : 0;
                const unsigned long long rightValue = i < rightParts.size() ? rightParts[i] : 0;
                if (leftValue != rightValue)
                    return leftValue < rightValue;
            }
            return false;
        }
    }

    HttpAssetUpdater::HttpAssetUpdater(HttpAssetUpdaterSettings settings)
        : settings_(std::move(settings))
    {
    }

    UpdateCheckResult HttpAssetUpdater::CheckForUpdates()
    {
        UpdateCheckResult result;

        WinHttpClient client(HTTP_ASSET_UPDATER_USER_AGENT, settings_.timeoutMilliSeconds);
        const HttpGetResult response = client.GetString(settings_.manifestUrl);
        if (!response.ok)
        {
            result.status = UpdateCheckStatus::Failed;
            result.error  = "マニフェストを取得できませんでした: " + response.error;
            return result;
        }

        std::string parseError;
        if (!AssetManifest::TryParse(response.body, result.remote, parseError))
        {
            result.status = UpdateCheckStatus::Failed;
            result.error  = "マニフェストを読めませんでした: " + parseError;
            return result;
        }
        result.remoteJson = response.body;

        if (!result.remote.requiredClientVersion.empty() &&
            HttpAssetUpdaterIsOlderVersion(settings_.clientVersion, result.remote.requiredClientVersion))
        {
            result.status = UpdateCheckStatus::ClientTooOld;
            result.error  = "ゲーム本体が古いため更新できません (必要 " + result.remote.requiredClientVersion
                          + " / 現在 " + settings_.clientVersion + ")";
            return result;
        }

        AssetManifest installed;
        if (std::filesystem::exists(settings_.paths.installedState))
        {
            // 壊れた installed.json は「何も入っていない」扱いに倒して、全件を更新対象として出す
            std::string installedError;
            if (!AssetManifest::TryLoadFile(settings_.paths.installedState, installed, installedError))
                installed = AssetManifest();
        }

        result.diff   = ManifestDiff::Between(installed, result.remote);
        result.status = result.diff.IsUpToDate() ? UpdateCheckStatus::UpToDate : UpdateCheckStatus::UpdateAvailable;
        return result;
    }

    DownloadResult HttpAssetUpdater::Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken)
    {
        DownloadResult result;
        const std::vector<ManifestBlob> blobs = update.diff.BlobsToInstall();

        std::uint64_t totalBytes = 0;
        for (const ManifestBlob& blob : blobs)
            totalBytes += blob.size;
        progress.receivedBytes.store(0);
        progress.totalBytes.store(totalBytes);
        progress.finishedFiles.store(0);
        progress.totalFiles.store(static_cast<std::uint32_t>(blobs.size()));

        const std::filesystem::path filesDirectory = settings_.paths.StagedBlobDirectory();
        std::error_code error;
        std::filesystem::create_directories(filesDirectory, error);
        if (error)
        {
            result.error = "一時フォルダを作れません: " + WideToUtf8(filesDirectory.wstring());
            return result;
        }

        // 落とす分に加えて、適用時に Assets/ 側へ同じ量を書き出す
        const std::filesystem::space_info space = std::filesystem::space(filesDirectory, error);
        if (!error && space.available < totalBytes * 2)
        {
            result.error = "ディスクの空き容量が足りません";
            return result;
        }

        WinHttpClient client(HTTP_ASSET_UPDATER_USER_AGENT, settings_.timeoutMilliSeconds);
        for (const ManifestBlob& blob : blobs)
        {
            if (stopToken.stop_requested())
            {
                result.cancelled = true;
                return result;
            }

            // 前回途中で止まっていても、照合済みのものはそのまま使う
            const std::filesystem::path staged = settings_.paths.StagedBlobPath(blob.hash);
            if (Sha256OfFile(staged) != blob.hash)
            {
                std::filesystem::path part = staged;
                part += L".part";

                std::string lastError;
                bool        fetched = false;
                for (int attempt = 0; attempt < HTTP_ASSET_UPDATER_ATTEMPTS && !fetched; ++attempt)
                {
                    if (attempt > 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(HTTP_ASSET_UPDATER_RETRY_WAIT_MSEC * attempt));

                    std::uint64_t received = 0;
                    const HttpDownloadResult response = client.DownloadToFile(update.remote.baseUrl + blob.hash, part, stopToken,
                        [&](const std::uint64_t bytes)
                        {
                            received += bytes;
                            progress.receivedBytes.fetch_add(bytes);
                        });
                    progress.receivedBytes.fetch_sub(received);

                    if (response.cancelled)
                    {
                        std::filesystem::remove(part, error);
                        result.cancelled = true;
                        return result;
                    }
                    if (!response.ok)
                    {
                        lastError = response.error;
                        continue;
                    }
                    if (Sha256OfFile(part) != blob.hash)
                    {
                        lastError = "中身がマニフェストと一致しません";
                        continue;
                    }
                    std::filesystem::rename(part, staged, error);
                    if (error)
                    {
                        lastError = "一時フォルダに保存できません";
                        continue;
                    }
                    fetched = true;
                }

                std::filesystem::remove(part, error);
                if (!fetched)
                {
                    result.error = blob.path + " を取得できませんでした: " + lastError;
                    return result;
                }
            }

            progress.receivedBytes.fetch_add(blob.size);
            progress.finishedFiles.fetch_add(1);
        }

        result.ok = true;
        return result;
    }
}
