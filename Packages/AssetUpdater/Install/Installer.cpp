#include "Installer.h"

#include <fstream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <windows.h>

#include "PathGuard.h"
#include "../Hash/Sha256.h"
#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr wchar_t INSTALLER_NEW_SUFFIX[] = L".update-new";
        constexpr wchar_t INSTALLER_OLD_SUFFIX[] = L".update-old";
        constexpr int     INSTALLER_MOVE_ATTEMPTS        = 5;
        constexpr DWORD   INSTALLER_MOVE_RETRY_WAIT_MSEC = 100;

        struct InstallerFile
        {
            std::filesystem::path target;
            std::string           hash;
            std::string           manifestPath;
        };

        struct InstallerJournal
        {
            std::filesystem::path target;
            bool                  movedAway = false;
            bool                  placed    = false;
        };

        std::filesystem::path InstallerSuffixed(const std::filesystem::path& path, const wchar_t* suffix)
        {
            std::filesystem::path result = path;
            result += suffix;
            return result;
        }

        // ウイルス対策ソフトや検索インデクサが一瞬だけ掴んでいることがあるので、少し待って再試行する
        bool InstallerMove(const std::filesystem::path& from, const std::filesystem::path& to, const DWORD flags)
        {
            for (int attempt = 0; attempt < INSTALLER_MOVE_ATTEMPTS; ++attempt)
            {
                if (MoveFileExW(from.c_str(), to.c_str(), flags))
                    return true;

                const DWORD reason = GetLastError();
                if (reason != ERROR_SHARING_VIOLATION && reason != ERROR_ACCESS_DENIED && reason != ERROR_LOCK_VIOLATION)
                    return false;
                Sleep(INSTALLER_MOVE_RETRY_WAIT_MSEC);
            }
            return false;
        }

        void InstallerRollback(const std::vector<InstallerJournal>& journal)
        {
            for (auto it = journal.rbegin(); it != journal.rend(); ++it)
            {
                if (it->placed)
                    DeleteFileW(it->target.c_str());
                if (it->movedAway)
                    InstallerMove(InstallerSuffixed(it->target, INSTALLER_OLD_SUFFIX), it->target, 0);
            }
        }

        void InstallerDiscardPrepared(const std::vector<InstallerFile>& files)
        {
            for (const InstallerFile& file : files)
                DeleteFileW(InstallerSuffixed(file.target, INSTALLER_NEW_SUFFIX).c_str());
        }

        bool InstallerWriteFile(const std::filesystem::path& path, const std::string& content)
        {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            stream.write(content.data(), static_cast<std::streamsize>(content.size()));
            stream.close();
            return static_cast<bool>(stream);
        }
    }

    Installer::Installer(AssetUpdaterPaths paths)
        : paths_(std::move(paths))
    {
    }

    ApplyResult Installer::Apply(const UpdateCheckResult& update) const
    {
        ApplyResult result;
        std::error_code error;
        if (!std::filesystem::exists(paths_.installedState, error))
        {
            result.error = "installed.json が無いので、配布版として扱えません";
            return result;
        }
        if (update.remoteJson.empty())
        {
            result.error = "適用するマニフェストがありません";
            return result;
        }

        // 1. 置き換えるファイルと消すファイルを決める。Assets/ の外を指すものが1つでもあれば何もしない
        std::vector<InstallerFile> files;
        const auto addFile = [&](const std::string& manifestPath, const std::string& hash)
        {
            const std::optional<std::filesystem::path> target = ResolveAssetPath(paths_.gameRoot, manifestPath);
            if (!target)
                return false;
            files.push_back({*target, hash, manifestPath});
            return true;
        };
        for (const std::vector<ManifestEntry>* group : {&update.diff.added, &update.diff.changed})
        {
            for (const ManifestEntry& entry : *group)
            {
                const bool accepted = addFile(entry.path, entry.hash)
                                   && (entry.metaHash.empty() || addFile(entry.path + ".meta", entry.metaHash));
                if (!accepted)
                {
                    result.error = "Assets/ の外を指すパスが含まれています: " + entry.path;
                    return result;
                }
            }
        }

        std::vector<std::filesystem::path> removals;
        for (const std::string& removedPath : update.diff.removedPaths)
        {
            for (const std::string& candidate : {removedPath, removedPath + ".meta"})
            {
                const std::optional<std::filesystem::path> target = ResolveAssetPath(paths_.gameRoot, candidate);
                if (!target)
                {
                    result.error = "Assets/ の外を指すパスが含まれています: " + removedPath;
                    return result;
                }
                if (std::filesystem::exists(*target, error))
                    removals.push_back(*target);
            }
        }

        // 2. 一時置き場のファイルを照合する
        for (const InstallerFile& file : files)
        {
            if (Sha256OfFile(paths_.StagedBlobPath(file.hash)) != file.hash)
            {
                result.error = "ダウンロード済みのファイルが見つからないか、壊れています: " + file.manifestPath;
                return result;
            }
        }

        // 3. 準備: 全部 .update-new として書き出す。ここで失敗しても Assets/ は変わっていない
        for (const InstallerFile& file : files)
        {
            std::filesystem::create_directories(file.target.parent_path(), error);
            const std::filesystem::path prepared = InstallerSuffixed(file.target, INSTALLER_NEW_SUFFIX);
            if (error || !CopyFileW(paths_.StagedBlobPath(file.hash).c_str(), prepared.c_str(), FALSE))
            {
                InstallerDiscardPrepared(files);
                result.error = "書き込めませんでした: " + file.manifestPath;
                return result;
            }
        }

        // 4. 入れ替え: 1つでも失敗したら、それまでの入れ替えを全部戻す
        std::vector<InstallerJournal> journal;
        journal.reserve(files.size() + removals.size());
        const auto fail = [&](const std::string& message)
        {
            InstallerRollback(journal);
            InstallerDiscardPrepared(files);
            result.error = message;
        };

        for (const InstallerFile& file : files)
        {
            InstallerJournal step{file.target};
            if (std::filesystem::exists(file.target, error))
            {
                if (!InstallerMove(file.target, InstallerSuffixed(file.target, INSTALLER_OLD_SUFFIX), MOVEFILE_REPLACE_EXISTING))
                {
                    fail("使用中のため置き換えられませんでした: " + file.manifestPath);
                    return result;
                }
                step.movedAway = true;
            }
            if (!InstallerMove(InstallerSuffixed(file.target, INSTALLER_NEW_SUFFIX), file.target, 0))
            {
                journal.push_back(step);
                fail("置き換えられませんでした: " + file.manifestPath);
                return result;
            }
            step.placed = true;
            journal.push_back(step);
        }

        for (const std::filesystem::path& removal : removals)
        {
            if (!InstallerMove(removal, InstallerSuffixed(removal, INSTALLER_OLD_SUFFIX), MOVEFILE_REPLACE_EXISTING))
            {
                fail("使用中のため削除できませんでした: " + WideToUtf8(removal.wstring()));
                return result;
            }
            journal.push_back({removal, true, false});
        }

        // 5. 確定: installed.json を書き換えた時点で、この版が入ったことになる
        const std::filesystem::path statePrepared = InstallerSuffixed(paths_.installedState, INSTALLER_NEW_SUFFIX);
        if (!InstallerWriteFile(statePrepared, update.remoteJson)
            || !InstallerMove(statePrepared, paths_.installedState, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(statePrepared.c_str());
            fail("installed.json を更新できませんでした");
            return result;
        }

        // 6. 片付け。失敗しても結果は変わらず、残ったものは次回の RemoveLeftovers が消す
        for (const InstallerJournal& step : journal)
        {
            if (step.movedAway)
                DeleteFileW(InstallerSuffixed(step.target, INSTALLER_OLD_SUFFIX).c_str());
        }
        std::filesystem::remove_all(paths_.stagingDirectory, error);

        result.ok = true;
        return result;
    }

    void Installer::RemoveLeftovers() const
    {
        std::error_code error;
        if (!std::filesystem::exists(paths_.installedState, error))
            return;

        DeleteFileW(InstallerSuffixed(paths_.installedState, INSTALLER_NEW_SUFFIX).c_str());

        const std::wstring_view newSuffix = INSTALLER_NEW_SUFFIX;
        const std::wstring_view oldSuffix = INSTALLER_OLD_SUFFIX;
        std::vector<std::filesystem::path> leftovers;
        for (auto it = std::filesystem::recursive_directory_iterator(paths_.gameRoot / L"Assets", error);
             !error && it != std::filesystem::recursive_directory_iterator();
             it.increment(error))
        {
            const std::wstring name = it->path().filename().wstring();
            if (name.ends_with(newSuffix) || name.ends_with(oldSuffix))
                leftovers.push_back(it->path());
        }
        for (const std::filesystem::path& leftover : leftovers)
            DeleteFileW(leftover.c_str());
    }
}
