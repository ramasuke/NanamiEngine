#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <cstdint>

namespace NanamiEngine::AssetUpdater
{
    /** ダウンロード中に別スレッドから書かれ、画面側が毎フレーム読む */
    struct NANAMI_API DownloadProgress
    {
        std::atomic<std::uint64_t> receivedBytes {0};
        std::atomic<std::uint64_t> totalBytes    {0};
        std::atomic<std::uint32_t> finishedFiles {0};
        std::atomic<std::uint32_t> totalFiles    {0};
    };
}
