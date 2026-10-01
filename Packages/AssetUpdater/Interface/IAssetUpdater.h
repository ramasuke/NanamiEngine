#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <stop_token>

#include "DownloadProgress.h"
#include "DownloadResult.h"
#include "UpdateCheckResult.h"

namespace NanamiEngine::AssetUpdater
{
    class NANAMI_API IAssetUpdater
    {
    public:
        virtual ~IAssetUpdater();

        [[nodiscard]] virtual UpdateCheckResult CheckForUpdates() = 0;
        [[nodiscard]] virtual DownloadResult Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken) = 0;
    };
}
