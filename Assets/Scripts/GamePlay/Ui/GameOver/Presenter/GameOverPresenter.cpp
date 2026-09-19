#include "GameOverPresenter.h"

#include <algorithm>

#include "DxLib.h"
#include "../Ui_GameOverScreen.h"
#include "../DeathCamera/GameOverDeathCamera.h"
#include "../../../Sound/SoundPlayer.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short GAME_OVER_STICK_DEADZONE = 12000;
    }

    void GameOverPresenter::OnStart()
    {
        view_ = RequireComponent<GameOverScreenUi>();
        lastTickMs_ = GetNowCount();

        for (const int index : {GameOverScreenUi::RETRY_INDEX, GameOverScreenUi::TITLE_INDEX})
        {
            const auto button = view_->ChoiceButton(index);
            if (!button)
                continue;

            button->OnHover().subscribe(DestroyCancellationToken(), [this, index](LibCore::Rx::unit)
            {
                if (phase_ == Phase::Presenting && view_->IsInputReady())
                    Select(index);
            });
            button->OnClick().subscribe(DestroyCancellationToken(), [this, index](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Presenting && view_->IsInputReady())
                    Decide(index);
            });
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
            // 別の経路でシーンが切り替わった（デバッグの遷移ボタン等）
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
                RequestSceneChange(false);
            }
            break;

        case Phase::LeavingByCurtain:
            if (view_->IsCurtainClosed())
                RequestSceneChange(true);
            break;

        case Phase::WaitingSceneChange:
            if (GameCore::Game::Instance().Scenes().HasPendingChange())
                break;

            holdSecs_ += deltaSecs;
            if (isCurtainUsed_ && holdSecs_ < curtainHoldSecs_)
                break;

            if (isCurtainUsed_)
                view_->OpenCurtain();
            fallenSecs_ = 0.0f;
            phase_ = Phase::Watching;
            break;
        }
    }

    float GameOverPresenter::TickWallClockSeconds()
    {
        const int nowMs = GetNowCount();
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
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);

        const bool isPrevPressed = CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A)
            || xInput.Buttons[XINPUT_BUTTON_DPAD_LEFT]
            || xInput.ThumbLX < -GAME_OVER_STICK_DEADZONE;
        const bool isNextPressed = CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)
            || xInput.Buttons[XINPUT_BUTTON_DPAD_RIGHT]
            || xInput.ThumbLX > GAME_OVER_STICK_DEADZONE;
        const bool isConfirmPressed = CheckHitKey(KEY_INPUT_RETURN) || CheckHitKey(KEY_INPUT_SPACE)
            || xInput.Buttons[XINPUT_BUTTON_A];

        // 石版が出切るまでは押下の記録だけ取る。倒れる直前から押しっぱなしのキーで決定させない
        if (view_->IsInputReady())
        {
            if (isPrevPressed && !wasPrevPressed_)
                Select(GameOverScreenUi::RETRY_INDEX);
            if (isNextPressed && !wasNextPressed_)
                Select(GameOverScreenUi::TITLE_INDEX);
            if (isConfirmPressed && !wasConfirmPressed_)
                Decide(selection_);
        }

        wasPrevPressed_    = isPrevPressed;
        wasNextPressed_    = isNextPressed;
        wasConfirmPressed_ = isConfirmPressed;
    }

    void GameOverPresenter::BeginGameOver()
    {
        phase_ = Phase::Presenting;
        selection_ = GameOverScreenUi::RETRY_INDEX;
        wasPrevPressed_ = true;
        wasNextPressed_ = true;
        wasConfirmPressed_ = true;

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
        selection_ = index;
        view_->SetSelection(index);
    }

    void GameOverPresenter::Decide(const int index)
    {
        Select(index);
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

        pendingSceneType_ = *currentSceneType;

        // ステージ選択から入ったステージは、ロード画面の手順(Show→覆い切ってから遷移)でしか明けない
        auto& loadingScreen = GameCore::Game::Instance().LoadingScreen();
        const auto stageData = loadingScreen.ShownStageData();
        if (stageData && stageData->SceneType() == pendingSceneType_)
        {
            loadingScreen.Show(stageData);
            phase_ = Phase::LeavingByLoading;
            return;
        }

        view_->BeginCurtain();
        phase_ = Phase::LeavingByCurtain;
    }

    void GameOverPresenter::ReturnToTitle()
    {
        pendingSceneType_ = GameCore::Scene::Main::SceneType::Title;
        view_->BeginCurtain();
        phase_ = Phase::LeavingByCurtain;
    }

    void GameOverPresenter::RequestSceneChange(const bool isCurtainUsed)
    {
        GameCore::Game::Instance().Scenes().RequestChangeScene(pendingSceneType_);
        isCurtainUsed_ = isCurtainUsed;
        holdSecs_ = 0.0f;
        phase_ = Phase::WaitingSceneChange;
    }

    void GameOverPresenter::Abort()
    {
        view_->HideImmediately();
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
    }
}
