#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "UpdateCheckStatus.h"
#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API UpdateCheckResult
    {
        UpdateCheckStatus status = UpdateCheckStatus::Failed;
        AssetManifest     remote;
        /** 取得したマニフェストそのもの。適用できたら installed.json としてこのまま書く */
        std::string       remoteJson;
        ManifestDiff      diff;
        std::string       error;
    };
}
