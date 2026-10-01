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
        std::string       remoteJson;
        ManifestDiff      diff;
        std::string       error;
    };
}
