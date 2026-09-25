#include "AssetUpdatePresenter.h"

#include <algorithm>
#include <filesystem>
#include <string>

#include "Assets/Scripts/Core/Input/InputAliases.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/Configuration/Build/ApplicationConfiguration_Build.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Packages/AssetUpdater/Http/HttpAssetUpdater.h"
#include "Packages/AssetUpdater/Null/NullAssetUpdater.h"
#include "Packages/AssetUpdater/System/Relaunch.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // exe に焼き込まれる。配信先を差し替えるときは tools/dist/dist_config.json と揃えて新しいビルドを出す
        constexpr const char* ASSET_UPDATE_MANIFEST_URL    = "https://pub-10484db77a4e4777b87c30443f6136c0.r2.dev/manifest.json";
        constexpr int         ASSET_UPDATE_HTTP_TIMEOUT_MS = 5000;

        // エディタの Preview で使う偽の荷
        constexpr float ASSET_UPDATE_PREVIEW_DOWNLOAD_SECS = 6.0f;
        // 受け取り終えてから入れ終えた札に移るまで
        constexpr float ASSET_UPDATE_PREVIEW_UNPACK_SECS = 1.0f;
        constexpr std::uint64_t ASSET_UPDATE_PREVIEW_BYTES = 327'576'781;   // 312.4 MB
        constexpr std::uint64_t ASSET_UPDATE_PREVIEW_FILES = 147;

        std::string AssetUpdateAmountText(const std::uint64_t received, const std::uint64_t total,
                                          const std::uint32_t finishedFiles, const std::uint32_t totalFiles)
        {
            return FormatAssetUpdateBytes(received) + " / " + FormatAssetUpdateBytes(total) + "    "
                 + std::to_string(finishedFiles) + " / " + std::to_string(totalFiles) + " 件";
        }
    }

    void AssetUpdatePresenter::OnStart()
    {
        view_ = RequireComponent<AssetUpdateTagUi>();
        view_->SubscribeHintClicks([this] { Confirm(); }, [this] { Cancel(); });

        const std::filesystem::path           gameRoot = std::filesystem::current_path();
        const AssetUpdater::AssetUpdaterPaths paths{gameRoot, gameRoot / "installed.json", gameRoot / ".update"};
        task_ = std::make_unique<AssetUpdater::AssetUpdateTask>(CreateAssetUpdater(paths), paths);
        task_->BeginCheck();

        // タイトルに入った瞬間の押しっぱなしを、札への答えとして拾わない
        previousKeys_ = ReadKeys();
    }

    std::unique_ptr<AssetUpdater::IAssetUpdater> AssetUpdatePresenter::CreateAssetUpdater(const AssetUpdater::AssetUpdaterPaths& paths) const
    {
        using NanamiEngine::Core::Application::Configuration::APPLICATION_MODE;
        using NanamiEngine::Core::Application::Configuration::ApplicationMode;
        using NanamiEngine::Core::Application::Configuration::BuildConfiguration;

        // エディタで動かすと、開発中の Assets/ が配信の中身で上書きされ、まだ上げていないファイルが消える
        if constexpr (APPLICATION_MODE == ApplicationMode::Game)
        {
            AssetUpdater::HttpAssetUpdaterSettings settings;
            settings.manifestUrl         = ASSET_UPDATE_MANIFEST_URL;
            settings.paths               = paths;
            settings.clientVersion       = BuildConfiguration::ClientVersion();
            settings.timeoutMilliSeconds = ASSET_UPDATE_HTTP_TIMEOUT_MS;
            return std::make_unique<AssetUpdater::HttpAssetUpdater>(settings);
        }
        else
        {
            return std::make_unique<AssetUpdater::NullAssetUpdater>();
        }
    }

    void AssetUpdatePresenter::OnUpdate()
    {
        if (!task_ || !view_)
            return;

        const Keys keys = ReadKeys();
        const bool isConfirmPressed = keys.confirm && !previousKeys_.confirm;
        const bool isCancelPressed  = keys.cancel && !previousKeys_.cancel;
        previousKeys_ = keys;

        if (preview_ != Preview::None)
        {
            UpdatePreview();
        }
        else
        {
            const AssetUpdater::AssetUpdateState state = task_->State();
            if (state != shownState_)
            {
                shownState_ = state;
                OnStateChanged(state);
            }
            if (state == AssetUpdater::AssetUpdateState::Downloading)
                ShowProgress();
        }

        if (isConfirmPressed)
            Confirm();
        else if (isCancelPressed)
            Cancel();
    }

    bool AssetUpdatePresenter::TryStartGame()
    {
        if (!task_)
            return false;
        if (task_->CanStartGame())
            return true;

        using AssetUpdater::AssetUpdateState;
        switch (const AssetUpdateState state = task_->State())
        {
        case AssetUpdateState::UpdateAvailable:
        case AssetUpdateState::Failed:
        case AssetUpdateState::ClientTooOld:
        case AssetUpdateState::ReadyToRestart:
            if (!view_->IsShown())
                ShowPromptFor(state);
            return false;
        default:
            // 確認中・ダウンロード中・適用中は受け付けない
            return false;
        }
    }

    AssetUpdatePresenter::Keys AssetUpdatePresenter::ReadKeys()
    {
        const auto xInput = Gamepad::Get();

        return Keys{
            .confirm = Keyboard::IsDown(Key::Return) || xInput.IsDown(GamepadButton::A),
            .cancel  = Keyboard::IsDown(Key::Escape) || xInput.IsDown(GamepadButton::B),
        };
    }

    void AssetUpdatePresenter::OnStateChanged(const AssetUpdater::AssetUpdateState state)
    {
        using AssetUpdater::AssetUpdateState;
        switch (state)
        {
        case AssetUpdateState::NotInstalled:
            Module::Log("[AssetUpdater] installed.json が無いので、配信の更新は確認しません");
            return;
        case AssetUpdateState::CheckFailed:
            Module::LogWarning("[AssetUpdater] 更新を確認できませんでした。今のアセットで遊べます: " + task_->ErrorMessage());
            return;
        case AssetUpdateState::UpToDate:
            Module::Log("[AssetUpdater] 最新です (" + task_->CheckResult().remote.version + ")");
            return;
        case AssetUpdateState::UpdateAvailable:
        {
            const AssetUpdater::UpdateCheckResult& check = task_->CheckResult();
            Module::Log("[AssetUpdater] 更新があります: " + check.remote.version + " / " + std::to_string(check.diff.UpdateCount())
                      + " 件 / " + FormatAssetUpdateBytes(check.diff.downloadBytes));
            ShowPromptFor(state);
            return;
        }
        case AssetUpdateState::ClientTooOld:
            Module::LogWarning("[AssetUpdater] " + task_->ErrorMessage());
            ShowPromptFor(state);
            return;
        case AssetUpdateState::Failed:
            Module::LogWarning("[AssetUpdater] 更新できませんでした: " + task_->ErrorMessage());
            ShowPromptFor(state);
            return;
        case AssetUpdateState::Downloading:
            view_->ShowReceiving();
            return;
        case AssetUpdateState::Applying:
            view_->ShowUnpacking();
            return;
        case AssetUpdateState::ReadyToRestart:
            Module::Log("[AssetUpdater] 更新が終わりました");
            canRelaunch_ = AssetUpdater::Relauncher::ScheduleOnExit();
            ShowPromptFor(state);
            return;
        case AssetUpdateState::Idle:
        case AssetUpdateState::Checking:
            return;
        }
    }

    void AssetUpdatePresenter::ShowPromptFor(const AssetUpdater::AssetUpdateState state)
    {
        using AssetUpdater::AssetUpdateState;
        switch (state)
        {
        case AssetUpdateState::UpdateAvailable:
            view_->ShowOffer(Parcel());
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);
            return;
        case AssetUpdateState::Failed:
            view_->ShowUndelivered(task_->ErrorMessage());
            PlaySound(stampSound_);
            return;
        case AssetUpdateState::ClientTooOld:
            view_->ShowWrongVersion(task_->ErrorMessage());
            PlaySound(stampSound_);
            return;
        case AssetUpdateState::ReadyToRestart:
            view_->ShowReceived(Parcel(), canRelaunch_);
            PlaySound(stampSound_);
            return;
        default:
            return;
        }
    }

    void AssetUpdatePresenter::ShowProgress()
    {
        const AssetUpdater::DownloadProgress& progress = task_->Progress();
        const std::uint64_t total    = progress.totalBytes.load();
        const std::uint64_t received = (std::min)(progress.receivedBytes.load(), total);
        const float         rate     = total == 0 ? 0.0f : static_cast<float>(static_cast<double>(received) / static_cast<double>(total));
        view_->SetProgress(rate, AssetUpdateAmountText(received, total, progress.finishedFiles.load(), progress.totalFiles.load()));
    }

    void AssetUpdatePresenter::Confirm()
    {
        if (!view_->IsShown() || !view_->HasConfirm())
            return;

        PlaySound(confirmSound_);
        switch (preview_)
        {
        case Preview::Offer:
        case Preview::Undelivered:
            preview_ = Preview::Receiving;
            PlayPreviewDownload();
            view_->ShowReceiving();
            return;
        case Preview::Received:
            // エディタで再起動はしない。札を引っ込めるだけ
            preview_ = Preview::None;
            view_->Hide();
            return;
        case Preview::Receiving:
        case Preview::WrongVersion:
            return;
        case Preview::None:
            break;
        }

        using AssetUpdater::AssetUpdateState;
        switch (task_->State())
        {
        case AssetUpdateState::UpdateAvailable:
        case AssetUpdateState::Failed:
            task_->BeginInstall();
            return;
        case AssetUpdateState::ReadyToRestart:
            Relaunch();
            return;
        default:
            return;
        }
    }

    void AssetUpdatePresenter::Cancel()
    {
        if (!view_->IsShown() || !view_->HasCancel())
            return;

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cancel);
        preview_ = Preview::None;
        view_->Hide();
    }

    void AssetUpdatePresenter::Relaunch() const
    {
        // 再起動の予約は ReadyToRestart に入ったときに済ませてある (できなかったら札で起動し直しを頼んでいる)。
        // 読み込み済みのアセットは古いままなので、このまま遊ばせずに終了する
        Core::Application::ApplicationBase::RequestClose();
    }

    AssetUpdateParcel AssetUpdatePresenter::Parcel() const
    {
        if (preview_ != Preview::None)
            return AssetUpdateParcel{ASSET_UPDATE_PREVIEW_FILES, ASSET_UPDATE_PREVIEW_BYTES, "0.9.3"};

        const AssetUpdater::UpdateCheckResult& check = task_->CheckResult();
        return AssetUpdateParcel{check.diff.UpdateCount(), check.diff.downloadBytes, check.remote.version};
    }

    void AssetUpdatePresenter::PlaySound(const FIELD(Asset::SoundFile)& sound) const
    {
        Sound::UiSoundBank::Play(sound.get());
    }

    void AssetUpdatePresenter::UpdatePreview()
    {
        if (preview_ != Preview::Receiving)
            return;

        // 偽の受け取り: 一定の速さで進め、終わったら入れ終えた札に移る
        const bool isFinished = previewDownloadTween_.Tick(Time::DeltaTime());
        const float rate = previewDownloadTween_.Value();
        const auto received = static_cast<std::uint64_t>(static_cast<double>(ASSET_UPDATE_PREVIEW_BYTES) * rate);
        const auto files = static_cast<std::uint32_t>(static_cast<float>(ASSET_UPDATE_PREVIEW_FILES) * rate);
        view_->SetProgress(rate, AssetUpdateAmountText(received, ASSET_UPDATE_PREVIEW_BYTES, files, ASSET_UPDATE_PREVIEW_FILES));
        if (isFinished)
        {
            preview_ = Preview::Received;
            view_->ShowReceived(Parcel(), true);
            PlaySound(stampSound_);
        }
    }

    void AssetUpdatePresenter::PlayPreviewDownload()
    {
        previewDownloadTween_.Play(tweeny::from(0.0f).to(1.0f).during(LibCore::Tween::Ms(ASSET_UPDATE_PREVIEW_DOWNLOAD_SECS))
            .to(1.0f).during(LibCore::Tween::Ms(ASSET_UPDATE_PREVIEW_UNPACK_SECS)));
    }

    void AssetUpdatePresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stampSound_", stampSound_);
        ImGuiHelper::OnDrawInputField("confirmSound_", confirmSound_);

        // エディタでは更新を確かめない (NullAssetUpdater) ので、見た目は偽の状態で確かめる
        if (!view_)
            return;
        ImGui::SeparatorText("Preview");
        const auto preview = [this](const Preview next, auto show)
        {
            preview_ = next;
            PlayPreviewDownload();
            show();
        };
        if (ImGui::Button("Offer"))
            preview(Preview::Offer, [this] { view_->ShowOffer(Parcel()); Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open); });
        ImGui::SameLine();
        if (ImGui::Button("Receiving"))
            preview(Preview::Receiving, [this] { view_->ShowReceiving(); });
        ImGui::SameLine();
        if (ImGui::Button("Undelivered"))
            preview(Preview::Undelivered, [this] { view_->ShowUndelivered("通信が途切れました (タイムアウト)"); PlaySound(stampSound_); });
        if (ImGui::Button("Received"))
            preview(Preview::Received, [this] { view_->ShowReceived(Parcel(), true); PlaySound(stampSound_); });
        ImGui::SameLine();
        if (ImGui::Button("WrongVersion"))
            preview(Preview::WrongVersion, [this]
            {
                view_->ShowWrongVersion("ゲーム本体が古いため更新できません (必要 1.1.0 / 今 1.0.0)");
                PlaySound(stampSound_);
            });
        ImGui::SameLine();
        if (ImGui::Button("Hide"))
            preview(Preview::None, [this] { view_->Hide(); });
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::AssetUpdatePresenter);
#pragma endregion
