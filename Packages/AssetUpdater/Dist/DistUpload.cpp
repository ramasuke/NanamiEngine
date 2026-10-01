#include "DistUpload.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <unordered_map>

#include <windows.h>

#include "DistExclusion.h"
#include "DistScan.h"
#include "../Hash/Sha256.h"
#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr std::string_view DIST_RUNTIME_LOCKED_SUFFIXES[] = { ".ttf", ".otf", ".ttc" };

        std::unordered_map<std::string, std::string> DistUploadLockedFiles(const AssetManifest& manifest)
        {
            std::unordered_map<std::string, std::string> locked;
            for (const ManifestEntry& entry : manifest.entries)
            {
                const std::string lowered = AsciiLower(entry.path);
                if (std::ranges::any_of(DIST_RUNTIME_LOCKED_SUFFIXES, [&lowered](const std::string_view suffix) { return lowered.ends_with(suffix); }))
                    locked[entry.path] = entry.hash;
            }
            return locked;
        }

        std::string DistUploadDisplayPath(std::filesystem::path path)
        {
            return WideToUtf8(path.make_preferred().wstring());
        }
    }

    std::map<std::string, DistBlob> BlobsOf(const AssetManifest& manifest, const std::filesystem::path& repoRoot)
    {
        std::map<std::string, DistBlob> blobs;
        for (const ManifestEntry& entry : manifest.entries)
        {
            blobs.try_emplace(entry.hash, DistBlob{ entry.hash, repoRoot / Utf8ToPath(entry.path), entry.size });
            if (!entry.metaHash.empty())
                blobs.try_emplace(entry.metaHash, DistBlob{ entry.metaHash, repoRoot / Utf8ToPath(entry.path + ".meta"), entry.metaSize });
        }
        return blobs;
    }

    std::vector<DistBlob> PlanUpload(const std::map<std::string, DistBlob>& blobs, const std::unordered_set<std::string>& existing)
    {
        std::vector<DistBlob> todo;
        for (const auto& [digest, blob] : blobs)
        {
            if (!existing.contains(digest))
                todo.push_back(blob);
        }
        return todo;
    }

    std::vector<std::string> StageBlobs(const std::vector<DistBlob>& blobs, const std::filesystem::path& stagingDir, const std::function<bool()>& isCanceled)
    {
        std::vector<std::string> problems;
        for (const DistBlob& blob : blobs)
        {
            if (isCanceled && isCanceled())
                break;

            std::error_code error;
            if (!std::filesystem::is_regular_file(blob.source, error))
            {
                problems.push_back("見つかりません: " + DistUploadDisplayPath(blob.source));
                continue;
            }

            const std::filesystem::path copyTo = stagingDir.empty() ? std::filesystem::path() : stagingDir / Utf8ToPath(blob.hash);
            const std::optional<std::string> digest = Sha256OfFileCopyingTo(blob.source, copyTo);
            if (digest == blob.hash)
                continue;
            if (!copyTo.empty())
                DeleteFileW(copyTo.c_str());
            if (!digest)
                problems.push_back("読めませんでした: " + DistUploadDisplayPath(blob.source));
            else
                problems.push_back("マニフェストと中身が違います (build 後に変更された?): " + DistUploadDisplayPath(blob.source));
        }
        return problems;
    }

    std::vector<std::string> LockedFileChanges(const AssetManifest& live, const AssetManifest& next)
    {
        const auto before = DistUploadLockedFiles(live);
        const auto after  = DistUploadLockedFiles(next);

        std::vector<std::string> changed;
        for (const auto& [path, digest] : before)
        {
            const auto found = after.find(path);
            if (found == after.end() || found->second != digest)
                changed.push_back(path);
        }
        std::ranges::sort(changed);
        return changed;
    }
}
