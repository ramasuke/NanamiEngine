#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    class DistHashCache;

    // NOTE: 配信の単位は Assets/ のツリーをそのまま鏡写しにしたもの。エントリは 2 種類ある
    struct NANAMI_API DistScanResult
    {
        std::vector<ManifestEntry> entries;
        std::vector<std::string>   excluded;
        std::vector<std::string>   foldedMeta;
        std::vector<std::string>   missingGuid;
        std::vector<std::string>   nonAscii;
        std::vector<std::string>   shiftJisMeta;
        std::string                error;
        bool                       canceled = false;

        [[nodiscard]] std::size_t   AssetCount    () const;
        [[nodiscard]] std::size_t   CompanionCount() const;
        [[nodiscard]] std::uint64_t TotalBytes    () const;
    };

    struct NANAMI_API DistMetaGuid
    {
        std::string guid;
        std::string encoding;
    };

    [[nodiscard]] NANAMI_API DistMetaGuid ReadMetaGuid(const std::filesystem::path& metaPath);

    [[nodiscard]] NANAMI_API std::string DecodeUtf8OrCp932(const std::string& bytes, bool* outWasCp932 = nullptr);

    // NOTE: ファイルシステムのパスを、リポジトリルートからの '/' 区切り UTF-8 にする
    [[nodiscard]] NANAMI_API std::string ToRelPosix(const std::filesystem::path& path, const std::filesystem::path& repoRoot);
    [[nodiscard]] NANAMI_API std::filesystem::path Utf8ToPath(const std::string& utf8);

    [[nodiscard]] NANAMI_API DistScanResult ScanAssets(const std::filesystem::path& assetsRoot, const std::filesystem::path& repoRoot,
                                                       DistHashCache& cache, const std::function<bool()>& isCanceled = {});
}
