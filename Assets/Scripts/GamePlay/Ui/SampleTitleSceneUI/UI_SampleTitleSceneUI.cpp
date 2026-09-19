#include "UI_SampleTitleSceneUI.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "DxLib.h"
#include "../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../Packages/AssetUpdater/Http/HttpAssetUpdater.h"
#include "../../../../../Packages/AssetUpdater/Null/NullAssetUpdater.h"
#include "../../../../../Packages/AssetUpdater/System/Relaunch.h"
#include "../../../../../Packages/AssetUpdater/Text/Utf8.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr const char* SAMPLE_TITLE_MANIFEST_URL          = "https://pub-10484db77a4e4777b87c30443f6136c0.r2.dev/manifest.json";
        constexpr const char* SAMPLE_TITLE_CLIENT_VERSION        = "1.0.0";
        constexpr int         SAMPLE_TITLE_HTTP_TIMEOUT_MS       = 5000;
        constexpr int         SAMPLE_TITLE_WINDOW_TITLE_CAPACITY = 256;
        constexpr wchar_t     SAMPLE_TITLE_DIALOG_CAPTION[]      = L"アセットの更新";

        std::string SampleTitleFormatBytes(const std::uint64_t bytes)
        {
            if (bytes < 1024)
                return std::to_string(bytes) + " B";
            if (bytes < 1024 * 1024)
                return std::to_string(bytes / 1024) + " KB";

            char formatted[32] = {};
            std::snprintf(formatted, sizeof(formatted), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
            return formatted;
        }

        int SampleTitleShowDialog(const std::string& message, const UINT type)
        {
            return MessageBoxW(GetMainWindowHandle(), AssetUpdater::Utf8ToWide(message).c_str(), SAMPLE_TITLE_DIALOG_CAPTION, type);
        }

        void SampleTitleShowClientTooOld(const std::string& error)
        {
            SampleTitleShowDialog(error + "\n\n新しい版のゲームをダウンロードしてください。", MB_OK | MB_ICONWARNING);
        }

        void SampleTitleSetWindowTitle(const std::string& title)
        {
            SetWindowTextW(GetMainWindowHandle(), AssetUpdater::Utf8ToWide(title).c_str());
        }
    }

    void SampleTitleScene::OnStart()
    {
        gameStartButton_->OnClick().subscribe(DestroyCancellationToken(), [this](NanamiUi::MouseState)
        {
            OnGameStart();
        });
        gameExitButton_ ->OnClick().subscribe(DestroyCancellationToken(), [this](NanamiUi::MouseState)
        {
        });

        wchar_t title[SAMPLE_TITLE_WINDOW_TITLE_CAPACITY] = {};
        GetWindowTextW(GetMainWindowHandle(), title, SAMPLE_TITLE_WINDOW_TITLE_CAPACITY);
        windowTitleBeforeUpdate_ = title;

        const std::filesystem::path           gameRoot = std::filesystem::current_path();
        const AssetUpdater::AssetUpdaterPaths paths{gameRoot, gameRoot / "installed.json", gameRoot / ".update"};
        assetUpdate_ = std::make_unique<AssetUpdater::AssetUpdateTask>(CreateAssetUpdater(paths), paths);
        assetUpdate_->BeginCheck();
    }

    void SampleTitleScene::OnUpdate()
    {
        if (!assetUpdate_)
            return;

        const AssetUpdater::AssetUpdateState state = assetUpdate_->State();
        if (state != shownAssetUpdateState_)
        {
            shownAssetUpdateState_ = state;
            OnAssetUpdateStateChanged(state);
        }
        if (state == AssetUpdater::AssetUpdateState::Downloading)
            ShowAssetUpdateProgress();
    }

    std::unique_ptr<AssetUpdater::IAssetUpdater> SampleTitleScene::CreateAssetUpdater(const AssetUpdater::AssetUpdaterPaths& paths) const
    {
        using NanamiEngine::Core::Application::Configuration::APPLICATION_MODE;
        using NanamiEngine::Core::Application::Configuration::ApplicationMode;

        // エディタで動かすと、開発中の Assets/ が配信の中身で上書きされ、まだ上げていないファイルが消える
        if constexpr (APPLICATION_MODE == ApplicationMode::Game)
        {
            AssetUpdater::HttpAssetUpdaterSettings settings;
            settings.manifestUrl         = SAMPLE_TITLE_MANIFEST_URL;
            settings.paths               = paths;
            settings.clientVersion       = SAMPLE_TITLE_CLIENT_VERSION;
            settings.timeoutMilliSeconds = SAMPLE_TITLE_HTTP_TIMEOUT_MS;
            return std::make_unique<AssetUpdater::HttpAssetUpdater>(settings);
        }
        else
        {
            return std::make_unique<AssetUpdater::NullAssetUpdater>();
        }
    }

    void SampleTitleScene::OnAssetUpdateStateChanged(const AssetUpdater::AssetUpdateState state)
    {
        using AssetUpdater::AssetUpdateState;
        switch (state)
        {
        case AssetUpdateState::NotInstalled:
            Module::Log("[AssetUpdater] installed.json が無いので、配信の更新は確認しません");
            return;
        case AssetUpdateState::CheckFailed:
            Module::LogWarning("[AssetUpdater] 更新を確認できませんでした。今のアセットで遊べます: " + assetUpdate_->ErrorMessage());
            return;
        case AssetUpdateState::UpToDate:
            Module::Log("[AssetUpdater] 最新です (" + assetUpdate_->CheckResult().remote.version + ")");
            return;
        case AssetUpdateState::ClientTooOld:
            SampleTitleShowClientTooOld(assetUpdate_->ErrorMessage());
            return;
        case AssetUpdateState::UpdateAvailable:
            AskAssetUpdate();
            return;
        case AssetUpdateState::Downloading:
            shownAssetUpdatePercent_ = -1;
            return;
        case AssetUpdateState::Applying:
            SampleTitleSetWindowTitle("アセットを適用しています…");
            return;
        case AssetUpdateState::Failed:
            RestoreWindowTitle();
            AskAssetUpdateRetry();
            return;
        case AssetUpdateState::ReadyToRestart:
            RestartAfterAssetUpdate();
            return;
        case AssetUpdateState::Idle:
        case AssetUpdateState::Checking:
            return;
        }
    }

    void SampleTitleScene::AskAssetUpdate()
    {
        const AssetUpdater::UpdateCheckResult& check = assetUpdate_->CheckResult();
        Module::Log("[AssetUpdater] 更新があります: " + check.remote.version + " / " + std::to_string(check.diff.UpdateCount())
                  + " 件 / " + SampleTitleFormatBytes(check.diff.downloadBytes));

        const std::string message = "新しいデータがあります（" + SampleTitleFormatBytes(check.diff.downloadBytes) + "）。\n"
                                    "ダウンロードして更新しますか？\n\n"
                                    "更新が終わると、ゲームは自動で再起動します。";
        if (SampleTitleShowDialog(message, MB_YESNO | MB_ICONINFORMATION) == IDYES)
            assetUpdate_->BeginInstall();
    }

    void SampleTitleScene::AskAssetUpdateRetry()
    {
        const std::string message = "更新できませんでした。\n\n" + assetUpdate_->ErrorMessage() + "\n\nもう一度試しますか？";
        if (SampleTitleShowDialog(message, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            assetUpdate_->BeginInstall();
    }

    void SampleTitleScene::ShowAssetUpdateProgress()
    {
        const AssetUpdater::DownloadProgress& progress = assetUpdate_->Progress();
        const std::uint64_t total    = progress.totalBytes.load();
        const std::uint64_t received = (std::min)(progress.receivedBytes.load(), total);
        const int           percent  = total == 0 ? 100 : static_cast<int>(received * 100 / total);
        if (percent == shownAssetUpdatePercent_)
            return;

        shownAssetUpdatePercent_ = percent;
        SampleTitleSetWindowTitle("アセットを更新しています " + std::to_string(percent) + "%  ("
                                + std::to_string(progress.finishedFiles.load()) + " / "
                                + std::to_string(progress.totalFiles.load()) + " ファイル)");
    }

    void SampleTitleScene::RestoreWindowTitle()
    {
        if (!windowTitleBeforeUpdate_.empty())
            SetWindowTextW(GetMainWindowHandle(), windowTitleBeforeUpdate_.c_str());
    }

    void SampleTitleScene::RestartAfterAssetUpdate()
    {
        SampleTitleSetWindowTitle("更新が終わりました");
        if (AssetUpdater::ScheduleRelaunchOnExit())
            SampleTitleShowDialog("更新が終わりました。ゲームを再起動します。", MB_OK | MB_ICONINFORMATION);
        else
            SampleTitleShowDialog("更新が終わりました。\nお手数ですが、ゲームを起動し直してください。", MB_OK | MB_ICONINFORMATION);

        // 読み込み済みのアセットは古いままなので、このまま遊ばせずに終了する
        PostMessageW(GetMainWindowHandle(), WM_CLOSE, 0, 0);
    }

    void SampleTitleScene::OnGameStart()
    {
        if (assetUpdate_ && !assetUpdate_->CanStartGame())
        {
            switch (assetUpdate_->State())
            {
            case AssetUpdater::AssetUpdateState::UpdateAvailable:
                AskAssetUpdate();
                return;
            case AssetUpdater::AssetUpdateState::Failed:
                AskAssetUpdateRetry();
                return;
            case AssetUpdater::AssetUpdateState::ClientTooOld:
                SampleTitleShowClientTooOld(assetUpdate_->ErrorMessage());
                return;
            default:
                // 確認中・ダウンロード中・適用中・再起動待ちは受け付けない
                return;
            }
        }

        switch (GameCore::LoadGameProgression())
        {
        case GameCore::GameProgresion::FirstTouchDownMainIsLand:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::FirstTouchDownMainIsLand);
            break;
        case GameCore::GameProgresion::MainIsland:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::MainIsland);
            break;
        case GameCore::GameProgresion::GrassLandStage:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::GrassLand);
            break;
        default:
            throw std::runtime_error("Gameの進行状況に応じたScene遷移が定義されていません。");
        }
    }

    void SampleTitleScene::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("gameStartButton_", gameStartButton_);
        ImGuiHelper::OnDrawInputField("gameExitButton_" , gameExitButton_);
    }
}
