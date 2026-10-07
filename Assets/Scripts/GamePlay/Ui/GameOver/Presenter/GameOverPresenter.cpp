#include "GameOverPresenter.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "../Ui_GameOverScreen.h"
#include "../DeathCamera/GameOverDeathCamera.h"
#include "../../../Sound/SoundPlayer.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void GameOverPresenter::OnStart()
    {
        view_ = RequireComponent<GameOverScreenUi>();
        lastTickMs_ = Time::NowMilliseconds();

        screen_ = RequireComponent<UiFlow::UiScreen>();
        screen_->Input().Map().AddKey(UiFlow::UiAction::Submit, Platform::Input::Key::Space);

        for (const int index : {GameOverScreenUi::RETRY_INDEX, GameOverScreenUi::TITLE_INDEX})
        {
            const auto button = view_->ChoiceButton(index);
            if (!button)
                continue;

            button->OnHover().Subscribe([this, index](R4::Unit)
            {
                if (phase_ == Phase::Presenting && view_->IsInputReady())
                    Select(index);
            }).AddTo(this);
            button->OnClick().Subscribe([this, index](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Presenting && view_->IsInputReady())
                    Decide(index);
            }).AddTo(this);
        }
    }

    void GameOverPresenter::OnUpdate()
    {
        const float deltaSecs = TickWallClockSeconds();
        if (!view_)
            return;

        switch (phase_)
        {
        case Phase::Watching:
            UpdateWatching(deltaSecs);
            break;

        case Phase::Presenting:
            if (!HasAnyPlayer())
            {
                Abort();
                break;
            }
            UpdateInput();
            break;

        case Phase::LeavingByLoading:
            if (GameCore::Game::Instance().LoadingScreen().IsCoverOpaque())
            {
                view_->HideImmediately();
                phase_ = Phase::WaitingSceneChange;
            }
            break;

        case Phase::WaitingSceneChange:
            if (GameCore::Game::Instance().Scenes().HasPendingChange())
                break;

            fallenSecs_ = 0.0f;
            phase_ = Phase::Watching;
            break;
        }
    }

    float GameOverPresenter::TickWallClockSeconds()
    {
        const int nowMs = Time::NowMilliseconds();
        const float deltaSecs = static_cast<float>(nowMs - lastTickMs_) / 1000.0f;
        lastTickMs_ = nowMs;
        return std::clamp(deltaSecs, 0.0f, 0.25f);
    }

    void GameOverPresenter::UpdateWatching(const float deltaSecs)
    {
        fallenSecs_ = AreAllPlayersFallen() ? fallenSecs_ + deltaSecs : 0.0f;
        if (fallenSecs_ >= fallenConfirmSecs_)
            BeginGameOver();
    }

    void GameOverPresenter::UpdateInput()
    {
        using UiFlow::UiAction;
        auto& input = screen_->Input();
        const bool isPrevPressed    = input.IsPressed(UiAction::Left);
        const bool isNextPressed    = input.IsPressed(UiAction::Right);
        const bool isConfirmPressed = input.IsPressed(UiAction::Submit);
        
        if (!view_->IsInputReady())
            return;

        if (isPrevPressed)
            Select(GameOverScreenUi::RETRY_INDEX);
        if (isNextPressed)
            Select(GameOverScreenUi::TITLE_INDEX);
        if (isConfirmPressed)
            Decide(selection_);
    }

    void GameOverPresenter::BeginGameOver()
    {
        phase_ = Phase::Presenting;
        selection_ = GameOverScreenUi::RETRY_INDEX;
        screen_->Open();

        Sound::SoundPlayer::StopAllBgm();
        view_->Show();
        StartDeathCamera();
    }

    void GameOverPresenter::StartDeathCamera() const
    {
        if (!deathCameraPrefab_)
            return;

        const auto owner = GameCore::PlayerAvatar::Owner();
        if (!owner)
            return;

        const auto cameraObject = NanamiEngine::Scene::GameObject::Instantiate(*deathCameraPrefab_.get()).lock();
        if (!cameraObject)
            return;

        if (const auto deathCamera = cameraObject->Components().Catch<GameOverDeathCamera>().lock())
            deathCamera->Begin(owner->PlayerTransform().GetGameObject());
    }

    void GameOverPresenter::Select(const int index)
    {
        if (index != selection_)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::StoneCursor);
        selection_ = index;
        view_->SetSelection(index);
    }

    void GameOverPresenter::Decide(const int index)
    {
        Select(index);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::StoneConfirm);
        if (index == GameOverScreenUi::RETRY_INDEX)
            Retry();
        else
            ReturnToTitle();
    }

    void GameOverPresenter::Retry()
    {
        const auto currentSceneType = GameCore::Game::Instance().Scenes().CurrentSceneType();
        if (!currentSceneType)
        {
            ReturnToTitle();
            return;
        }

        RequestSceneChange(*currentSceneType, { .isRetry = true });
    }

    void GameOverPresenter::ReturnToTitle()
    {
        RequestSceneChange(GameCore::Scene::Main::SceneType::Title);
    }

    void GameOverPresenter::RequestSceneChange(
        const GameCore::Scene::Main::SceneType sceneType,
        const GameCore::Scene::Main::SceneTransitionOptions options)
    {
        GameCore::Game::Instance().Scenes().RequestChangeScene(sceneType, options);
        screen_->Close();
        phase_ = Phase::LeavingByLoading;
    }

    void GameOverPresenter::Abort()
    {
        view_->HideImmediately();
        screen_->Close();
        fallenSecs_ = 0.0f;
        phase_ = Phase::Watching;
    }

    bool GameOverPresenter::AreAllPlayersFallen()
    {
        bool hasPlayer = false;
        for (const auto& weakAvatar : GameCore::IPlayerAvatar::PlayerAvatars())
        {
            const auto avatar = weakAvatar.lock();
            if (!avatar)
                continue;

            hasPlayer = true;
            if (!avatar->PlayerStatus().IsDeath())
                return false;
        }
        return hasPlayer;
    }

    bool GameOverPresenter::HasAnyPlayer()
    {
        const auto& avatars = GameCore::IPlayerAvatar::PlayerAvatars();
        return std::ranges::any_of(avatars, [](const std::weak_ptr<GameCore::IPlayerAvatar>& avatar)
        {
            return !avatar.expired();
        });
    }

    void GameOverPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("deathCameraPrefab_", deathCameraPrefab_);
        ImGuiHelper::OnDrawInputField("fallenConfirmSecs_", fallenConfirmSecs_);
        ImGuiHelper::OnDrawInputField("curtainHoldSecs_", curtainHoldSecs_);
        ImGui::Text("phase: %d  fallen: %.2f  selection: %d", static_cast<int>(phase_), fallenSecs_, selection_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverPresenter);
#pragma endregion
