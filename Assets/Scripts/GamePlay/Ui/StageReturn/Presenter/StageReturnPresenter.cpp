#include "StageReturnPresenter.h"

#include <algorithm>

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
#include "Packages/ControlLock/ControlLock.h"

namespace GamePlay::Ui
{
    void StageReturnPresenter::OnStart()
    {
        screen_ = RequireComponent<UiFlow::UiScreen>();
        view_   = RequireComponent<StageReturnNoticeUi>();

        screen_->OnCovered().Subscribe([this](R4::Unit)
        {
            view_->Hide();
        }).AddTo(this);
        screen_->OnRevealed().Subscribe([this](R4::Unit)
        {
            view_->Open(IsHostLeavingOthers(), selection_);
        }).AddTo(this);

        if (const auto button = view_->ConfirmButton())
        {
            button->OnClick().Subscribe([this](NanamiUi::MouseState)
            {
                if (screen_->IsFocused() && !isLeaving_)
                    Decide();
            }).AddTo(this);
        }
        if (const auto button = view_->CancelButton())
        {
            button->OnClick().Subscribe([this](NanamiUi::MouseState)
            {
                if (screen_->IsFocused() && !isLeaving_)
                    Close();
            }).AddTo(this);
        }
    }

    void StageReturnPresenter::OnUpdate()
    {
        if (!view_)
            return;

        using UiFlow::UiAction;
        const auto avatar = GameCore::PlayerAvatar::Owner();

        if (!screen_->IsOpen())
        {
            if (toggleInput_.IsPressed(UiAction::Menu) && avatar && CanOpen(*avatar))
                Open();
            return;
        }
        if (isLeaving_ || !screen_->IsFocused())
            return;

        if (!avatar)
        {
            Close(false);
            return;
        }

        auto& input = screen_->Input();
        if (input.IsPressed(UiAction::Up))
            Select(std::max(selection_ - 1, 0));
        if (input.IsPressed(UiAction::Down))
            Select(std::min(selection_ + 1, StageReturnNoticeUi::ROW_COUNT - 1));

        if (input.IsPressed(UiAction::Cancel) || input.IsPressed(UiAction::Menu))
            Close();
        else if (input.IsPressed(UiAction::Submit))
            Decide();
    }

    void StageReturnPresenter::Open()
    {
        if (!screen_->Open())
            return;

        // NOTE: 誤って決めても帰らないよう、開いたときは「まだ残る」
        selection_ = StageReturnNoticeUi::STAY_INDEX;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);
        view_->Open(IsHostLeavingOthers(), selection_);
    }

    void StageReturnPresenter::Close(const bool withSound)
    {
        if (withSound)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        view_->Hide();
        screen_->Close();
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

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Confirm);
        isLeaving_ = true;
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

        (void)SettingsScreenPresenter::Open(*prefab);
    }

    bool StageReturnPresenter::CanOpen(const GameCore::IPlayerAvatar& avatar)
    {
        return UiFlow::ScreenStack::Instance().IsEmpty()
            && !ControlLock::Service::Instance().IsLocked()
            && avatar.IsAcceptingControl()
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
        ImGui::Text("selection: %d  leaving: %d", selection_, isLeaving_ ? 1 : 0);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageReturnPresenter);
#pragma endregion
