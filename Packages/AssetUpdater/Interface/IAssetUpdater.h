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

        /** 差分を調べるだけで、ダウンロードはしない */
        [[nodiscard]] virtual UpdateCheckResult CheckForUpdates() = 0;
        /** 差分のファイルを一時置き場へ落として照合する。Assets/ には触らない */
        [[nodiscard]] virtual DownloadResult Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken) = 0;
    };
}
