#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "../Install/AssetUpdaterPaths.h"
#include "../Interface/IAssetUpdater.h"

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API HttpAssetUpdaterSettings
    {
        std::string       manifestUrl;
        AssetUpdaterPaths paths;
        std::string       clientVersion;
        int               timeoutMilliSeconds;
    };

    class NANAMI_API HttpAssetUpdater final : public IAssetUpdater
    {
    public:
        explicit HttpAssetUpdater(HttpAssetUpdaterSettings settings);

        [[nodiscard]] UpdateCheckResult CheckForUpdates() override;
        [[nodiscard]] DownloadResult Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken) override;

    private:
        HttpAssetUpdaterSettings settings_;
    };
}
