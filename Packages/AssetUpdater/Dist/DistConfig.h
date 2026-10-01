#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::AssetUpdater::Dist
{
    /** 配信先。ProjectConfig/Build/AssetDistribution/ に置き、コードにはハードコードしない */
    struct NANAMI_API DistConfig
    {
        /** rclone の remote とバケット (例 r2:nanami-assets) */
        std::string remote;
        /** プレイヤーが読みに来る公開 URL */
        std::string publicBaseUrl;
        std::string rclone = "rclone";

        /** manifest.json の baseUrl。クライアントは baseUrl + <hash> で本体を取りに来る */
        [[nodiscard]] std::string FilesBaseUrl() const;
        [[nodiscard]] std::string ManifestUrl () const;
    };
}
