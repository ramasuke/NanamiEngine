#include "AssetUpdateTask.h"

#include <filesystem>
#include <utility>


namespace NanamiEngine::AssetUpdater
{
    AssetUpdateTask::AssetUpdateTask(std::unique_ptr<IAssetUpdater> updater, AssetUpdaterPaths paths)
        : updater_(std::move(updater))
        , paths_(std::move(paths))
        , installer_(paths_)
    {
    }

    AssetUpdateTask::~AssetUpdateTask() = default;

    void AssetUpdateTask::BeginCheck()
    {
        const AssetUpdateState state = State();
        if (state == AssetUpdateState::Checking || state == AssetUpdateState::Downloading || state == AssetUpdateState::Applying)
            return;

        state_.store(AssetUpdateState::Checking, std::memory_order_release);
        worker_ = std::jthread([this] { RunCheck(); });
    }

    void AssetUpdateTask::BeginInstall()
    {
        const AssetUpdateState state = State();
        if (state != AssetUpdateState::UpdateAvailable && state != AssetUpdateState::Failed)
            return;

        state_.store(AssetUpdateState::Downloading, std::memory_order_release);
        worker_ = std::jthread([this](const std::stop_token stopToken) { RunInstall(stopToken); });
    }

    AssetUpdateState AssetUpdateTask::State() const
    {
        return state_.load(std::memory_order_acquire);
    }

    bool AssetUpdateTask::CanStartGame() const
    {
        const AssetUpdateState state = State();
        return state == AssetUpdateState::NotInstalled || state == AssetUpdateState::CheckFailed || state == AssetUpdateState::UpToDate;
    }

    const UpdateCheckResult& AssetUpdateTask::CheckResult() const
    {
        return checkResult_;
    }

    const std::string& AssetUpdateTask::ErrorMessage() const
    {
        return errorMessage_;
    }

    const DownloadProgress& AssetUpdateTask::Progress() const
    {
        return progress_;
    }

    void AssetUpdateTask::RunCheck()
    {
        std::error_code error;
        if (!std::filesystem::exists(paths_.installedState, error))
        {
            state_.store(AssetUpdateState::NotInstalled, std::memory_order_release);
            return;
        }

        installer_.RemoveLeftovers();
        checkResult_  = updater_->CheckForUpdates();
        errorMessage_ = checkResult_.error;

        AssetUpdateState next = AssetUpdateState::CheckFailed;
        switch (checkResult_.status)
        {
        case UpdateCheckStatus::UpToDate:        next = AssetUpdateState::UpToDate;        break;
        case UpdateCheckStatus::UpdateAvailable: next = AssetUpdateState::UpdateAvailable; break;
        case UpdateCheckStatus::ClientTooOld:    next = AssetUpdateState::ClientTooOld;    break;
        case UpdateCheckStatus::Failed:          next = AssetUpdateState::CheckFailed;     break;
        }
        state_.store(next, std::memory_order_release);
    }

    void AssetUpdateTask::RunInstall(const std::stop_token& stopToken)
    {
        const DownloadResult download = updater_->Download(checkResult_, progress_, stopToken);
        if (download.cancelled)
        {
            state_.store(AssetUpdateState::UpdateAvailable, std::memory_order_release);
            return;
        }
        if (!download.ok)
        {
            errorMessage_ = download.error;
            state_.store(AssetUpdateState::Failed, std::memory_order_release);
            return;
        }

        state_.store(AssetUpdateState::Applying, std::memory_order_release);
        const ApplyResult applied = installer_.Apply(checkResult_);
        if (!applied.ok)
        {
            errorMessage_ = applied.error;
            state_.store(AssetUpdateState::Failed, std::memory_order_release);
            return;
        }
        state_.store(AssetUpdateState::ReadyToRestart, std::memory_order_release);
    }
}
