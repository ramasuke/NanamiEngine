#include "NullAssetUpdater.h"

namespace NanamiEngine::AssetUpdater
{
    UpdateCheckResult NullAssetUpdater::CheckForUpdates()
    {
        UpdateCheckResult result;
        result.status = UpdateCheckStatus::UpToDate;
        return result;
    }

    DownloadResult NullAssetUpdater::Download(const UpdateCheckResult&, DownloadProgress&, const std::stop_token&)
    {
        DownloadResult result;
        result.ok = true;
        return result;
    }
}
