#pragma once

namespace NanamiEngine::AssetUpdater
{
    enum class UpdateCheckStatus
    {
        UpToDate,
        UpdateAvailable,
        ClientTooOld,
        Failed,
    };
}
