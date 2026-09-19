#pragma once
#include <atomic>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#include "../Install/AssetUpdaterPaths.h"
#include "../Interface/IAssetUpdater.h"

namespace NanamiEngine::AssetUpdater
{
    enum class AssetUpdateState
    {
        Idle,
        Checking,
        /** installed.json が無い (エディタや開発中のリポジトリ)。更新は一切しない */
        NotInstalled,
        /** オフラインなどで確認できなかった。今のアセットで遊べる */
        CheckFailed,
        UpToDate,
        ClientTooOld,
        /** 更新がある。確認してから BeginInstall する */
        UpdateAvailable,
        Downloading,
        Applying,
        /** 更新があるのに落とせなかった / 適用できなかった。遊ばせずに再試行させる */
        Failed,
        /** 適用済み。読み込み済みのアセットは古いので再起動する */
        ReadyToRestart,
    };

    /**
     * 確認・ダウンロード・適用を別スレッドで進める。呼び出し側は毎フレーム State() を見る。
     * CheckResult() / ErrorMessage() は Checking / Downloading / Applying の間は読まないこと
     */
    class AssetUpdateTask final
    {
    public:
        AssetUpdateTask(std::unique_ptr<IAssetUpdater> updater, AssetUpdaterPaths paths);
        ~AssetUpdateTask();

        AssetUpdateTask(const AssetUpdateTask&)            = delete;
        AssetUpdateTask& operator=(const AssetUpdateTask&) = delete;

        void BeginCheck();
        /** UpdateAvailable か Failed のときだけ受け付ける */
        void BeginInstall();

        [[nodiscard]] AssetUpdateState         State() const;
        [[nodiscard]] bool                     CanStartGame() const;
        [[nodiscard]] const UpdateCheckResult& CheckResult() const;
        [[nodiscard]] const std::string&       ErrorMessage() const;
        [[nodiscard]] const DownloadProgress&  Progress() const;

    private:
        void RunCheck();
        void RunInstall(const std::stop_token& stopToken);

        std::unique_ptr<IAssetUpdater> updater_;
        AssetUpdaterPaths              paths_;
        std::atomic<AssetUpdateState>  state_ {AssetUpdateState::Idle};
        UpdateCheckResult              checkResult_;
        std::string                    errorMessage_;
        DownloadProgress               progress_;
        // 最後に宣言して最初に破棄させる。走っている処理を止めて待ってから、ほかのメンバーが消える
        std::jthread                   worker_;
    };
}
