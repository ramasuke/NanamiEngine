#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API ManifestEntry
    {
        std::string   guid;
        std::string   path;
        std::string   hash;
        std::string   metaHash;
        std::uint64_t size     = 0;
        std::uint64_t metaSize = 0;

        [[nodiscard]] std::uint64_t TotalSize() const { return size + metaSize; }
    };

    struct NANAMI_API AssetManifest
    {
        int                        schema = 0;
        std::string                version;
        std::string                requiredClientVersion;
        std::string                baseUrl;
        std::vector<ManifestEntry> entries;

        [[nodiscard]] static bool TryParse   (const std::string& json,               AssetManifest& outManifest, std::string& outError);
        [[nodiscard]] static bool TryLoadFile(const std::filesystem::path& filePath, AssetManifest& outManifest, std::string& outError);
    };

    struct NANAMI_API ManifestBlob
    {
        std::string   hash;
        std::uint64_t size = 0;
        // NOTE: この中身を使うパスのうち最初の1つ。エラーの表示用
        std::string   path;
    };

    struct NANAMI_API ManifestDiff
    {
        std::vector<ManifestEntry> added;
        std::vector<ManifestEntry> changed;
        std::vector<std::string>   removedPaths;
        std::uint64_t              downloadBytes = 0;

        [[nodiscard]] static ManifestDiff Between(const AssetManifest& installed, const AssetManifest& remote);

        [[nodiscard]] bool        IsUpToDate()  const;
        [[nodiscard]] std::size_t UpdateCount() const;

        [[nodiscard]] std::vector<ManifestBlob> BlobsToInstall() const;
    };
}
