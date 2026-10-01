#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../Interface/IAssetUpdater.h"

namespace NanamiEngine::AssetUpdater
{
    class NANAMI_API NullAssetUpdater final : public IAssetUpdater
    {
    public:
        [[nodiscard]] UpdateCheckResult CheckForUpdates() override;
        [[nodiscard]] DownloadResult Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken) override;
    };
}
