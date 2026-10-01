#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <string>

#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    struct DistScanResult;

    /** AssetManifest::TryParse と InstalledState が読み書きする schema */
    constexpr int DIST_MANIFEST_SCHEMA = 1;

    [[nodiscard]] NANAMI_API AssetManifest BuildManifest(const DistScanResult& scan, const std::string& version, const std::string& requiredClientVersion, const std::string& baseUrl);

    /** UTF-8 / LF / BOM 無し / インデント 2 の JSON (rapidjson がそのまま読める形) */
    [[nodiscard]] NANAMI_API std::string SerializeManifest(const AssetManifest& manifest);

    /** 一時ファイルに書いてから置き換える。失敗したら理由を返す */
    [[nodiscard]] NANAMI_API std::string WriteManifestFile(const AssetManifest& manifest, const std::filesystem::path& path);

    [[nodiscard]] NANAMI_API std::uint64_t TotalBytesOf(const AssetManifest& manifest);
    [[nodiscard]] NANAMI_API std::string   FormatBytes (std::uint64_t count);
}
