#include "StageReturnPresenter.h"

#include <algorithm>

#include "Assets/Scripts/Core/Input/InputAliases.h"
#include "../Ui_StageReturnNotice.h"
#include "../../Settings/Presenter/SettingsScreenPresenter.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short STAGE_RETURN_STICK_DEADZONE = 12000;
    }

    void StageReturnPresenter::OnStart()
    {
        view_ = RequireComponent<StageReturnNoticeUi>();

        if (const auto button = view_->ConfirmButton())
        {
            button->OnClick().Subscribe([this](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Opened)
                    Decide();
            }).AddTo(this);
        }
        if (const auto button = view_->CancelButton())
        {
            button->OnClick().Subscribe([this](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Opened)
                    Close();
            }).AddTo(this);
        }
    }

    void StageReturnPresenter::OnUpdate()
    {
        if (!view_)
            return;

        const Keys keys = ReadKeys();
        const auto avatar = GameCore::PlayerAvatar::Owner();

        // 閉じた ESC / B をジャンプなどに拾わせないよう、State は次のフレームで戻す
        if (isResumePending_)
        {
            isResumePending_ = false;
            if (avatar)
                avatar->EnableStateMachiine();
        }

        switch (phase_)
        {
        case Phase::Closed:
            if (keys.toggle && !previousKeys_.toggle && avatar && CanOpen(*avatar))
                Open(*avatar);
            break;

        case Phase::Opened:
            if (!avatar)
                Close(false);
            else
                UpdateOpened(keys);
            break;

        case Phase::Settings:
            // NOTE: 設定を閉じた ESC / B でこちらまで閉じないよう、戻ったフレームは入力を見ない
            if (!SettingsScreenPresenter::IsOpen() || settings_.expired())
            {
                phase_ = Phase::Opened;
                view_->Open(IsHostLeavingOthers(), selection_);
            }
            break;

        case Phase::Leaving:
            break;
        }

        previousKeys_ = keys;
    }

    void StageReturnPresenter::UpdateOpened(const Keys& keys)
    {
        if (keys.prev && !previousKeys_.prev)
            Select(std::max(selection_ - 1, 0));
        if (keys.next && !previousKeys_.next)
            Select(std::min(selection_ + 1, StageReturnNoticeUi::ROW_COUNT - 1));

        if (keys.cancel && !previousKeys_.cancel)
            Close();
        else if (keys.confirm && !previousKeys_.confirm)
            Decide();
    }

    void StageReturnPresenter::Open(GameCore::IPlayerAvatar& avatar)
    {
        phase_ = Phase::Opened;
        // 誤って決めても帰らないよう、開いたときは「まだ残る」
        selection_ = StageReturnNoticeUi::STAY_INDEX;
        avatar.DisableStateMachine();
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);
        view_->Open(IsHostLeavingOthers(), selection_);
    }

    void StageReturnPresenter::Close(const bool withSound)
    {
        if (withSound)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        phase_ = Phase::Closed;
        isResumePending_ = true;
        view_->Hide();
    }

    void StageReturnPresenter::Select(const int index)
    {
        if (index == selection_)
            return;

        selection_ = index;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        view_->SetSelection(index);
    }

    void StageReturnPresenter::Decide()
    {
        if (selection_ == StageReturnNoticeUi::SETTINGS_INDEX)
        {
            OpenSettings();
            return;
        }
        if (selection_ != StageReturnNoticeUi::RETURN_INDEX)
        {
            Close();
            return;
        }

        // 貼り紙はロード画面の下に隠れるまで出したままにし、シーンごと片付けられる
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Confirm);
        phase_ = Phase::Leaving;
        GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::MainIsland);
    }

    void StageReturnPresenter::OpenSettings()
    {
        const auto prefab = settingsPrefab_.get();
        if (!prefab)
        {
            Module::LogWarning("[StageReturn] settingsPrefab_ が未設定なので、設定画面を開けません");
            return;
        }

        settings_ = SettingsScreenPresenter::Open(*prefab);
        if (settings_.expired())
            return;

        // プレイヤーの State は止めたまま、貼り紙だけを隠す
        phase_ = Phase::Settings;
        view_->Hide();
    }

    StageReturnPresenter::Keys StageReturnPresenter::ReadKeys()
    {
        const auto xInput = Gamepad::Get();

        return Keys{
            .toggle  = Keyboard::IsDown(Key::Escape) || xInput.IsDown(GamepadButton::Start),
            .prev    = Keyboard::IsDown(Key::Up) || Keyboard::IsDown(Key::W)
                       || xInput.IsDown(GamepadButton::DPadUp) || xInput.thumbLY > STAGE_RETURN_STICK_DEADZONE,
            .next    = Keyboard::IsDown(Key::Down) || Keyboard::IsDown(Key::S)
                       || xInput.IsDown(GamepadButton::DPadDown) || xInput.thumbLY < -STAGE_RETURN_STICK_DEADZONE,
            .confirm = Keyboard::IsDown(Key::Return) || xInput.IsDown(GamepadButton::A),
            .cancel  = Keyboard::IsDown(Key::Escape) || xInput.IsDown(GamepadButton::B)
                       || xInput.IsDown(GamepadButton::Start),
        };
    }

    bool StageReturnPresenter::CanOpen(const GameCore::IPlayerAvatar& avatar)
    {
        // 会話・店・掲示板などで State を止めている間と、倒れている間は出さない
        return avatar.IsAcceptingControl()
            && !avatar.PlayerStatus().IsDeath()
            && !GameCore::Game::Instance().Scenes().HasPendingChange();
    }

    bool StageReturnPresenter::IsHostLeavingOthers()
    {
        const auto* runner = NanamiEngine::Module::Network::NetworkRunnerBase::TryGetInstance();
        if (!runner || !runner->IsServer())
            return false;

        const auto& avatars = GameCore::IPlayerAvatar::PlayerAvatars();
        return std::ranges::any_of(avatars, [](const std::weak_ptr<GameCore::IPlayerAvatar>& weakAvatar)
        {
            const auto avatar = weakAvatar.lock();
            return avatar && !avatar->IsOwner();
        });
    }

    void StageReturnPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("settingsPrefab_", settingsPrefab_);
        ImGui::Text("phase: %d  selection: %d", static_cast<int>(phase_), selection_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageReturnPresenter);
#pragma endregion
