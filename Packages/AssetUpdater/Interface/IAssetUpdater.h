#pragma once
#include <atomic>
#include <cstdint>
#include <stop_token>
#include <string>

#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater
{
    enum class UpdateCheckStatus
    {
        UpToDate,
        UpdateAvailable,
        ClientTooOld,
        Failed,
    };

    struct UpdateCheckResult
    {
        UpdateCheckStatus status = UpdateCheckStatus::Failed;
        AssetManifest     remote;
        /** 取得したマニフェストそのもの。適用できたら installed.json としてこのまま書く */
        std::string       remoteJson;
        ManifestDiff      diff;
        std::string       error;
    };

    /** ダウンロード中に別スレッドから書かれ、画面側が毎フレーム読む */
    struct DownloadProgress
    {
        std::atomic<std::uint64_t> receivedBytes {0};
        std::atomic<std::uint64_t> totalBytes    {0};
        std::atomic<std::uint32_t> finishedFiles {0};
        std::atomic<std::uint32_t> totalFiles    {0};
    };

    struct DownloadResult
    {
        bool        ok        = false;
        bool        cancelled = false;
        std::string error;
    };

    class IAssetUpdater
    {
    public:
        virtual ~IAssetUpdater();

        /** 差分を調べるだけで、ダウンロードはしない */
        [[nodiscard]] virtual UpdateCheckResult CheckForUpdates() = 0;
        /** 差分のファイルを一時置き場へ落として照合する。Assets/ には触らない */
        [[nodiscard]] virtual DownloadResult Download(const UpdateCheckResult& update, DownloadProgress& progress, const std::stop_token& stopToken) = 0;
    };
}
