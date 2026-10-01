#include "TitleScreenPresenter.h"

#include <algorithm>

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../Ui_TitleScreen.h"
#include "../../AssetUpdate/Presenter/AssetUpdatePresenter.h"
#include "../../Settings/Presenter/SettingsScreenPresenter.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/MainProgression/MainProgression.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void TitleScreenPresenter::OnStart()
    {
        view_ = RequireComponent<TitleScreenUi>();

        using UiFlow::UiAction;
        using Platform::Input::GamepadButton;
        using Platform::Input::Key;
        screen_ = RequireComponent<UiFlow::UiScreen>();
        screen_->Open();
        screen_->Input().Map()
            .AddKey   (UiAction::Submit, Key::Space)
            .AddButton(UiAction::Submit, GamepadButton::Start)
            .AddKey   (UiAction::Cancel, Key::Back);
        screen_->OnRevealed().Subscribe([this](R4::Unit)
        {
            if (phase_ != Phase::Settings)
                return;
            view_->SetCovered(false);
            phase_ = Phase::Menu;
        }).AddTo(this);

        view_->SetStartLabel(GameCore::LoadGameProgression() == GameCore::GameProgresion::FirstTouchDownMainIsLand
            ? "はじめから" : "つづきから");
        view_->SetSelection(selection_);

        for (const int index : {TitleScreenUi::START_INDEX, TitleScreenUi::SETTINGS_INDEX, TitleScreenUi::EXIT_INDEX})
        {
            const auto button = view_->MenuButton(index);
            if (!button)
                continue;

            button->OnHover().Subscribe([this, index](R4::Unit)
            {
                if (phase_ == Phase::Menu && view_->IsMenuReady())
                {
                    Select(index);
                }
            }).AddTo(this);
            button->OnClick().Subscribe([this, index](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Menu && view_->IsMenuReady())
                {
                    Decide(index);
                }
            }).AddTo(this);
        }

        const auto prefab = assetUpdatePrefab_.get();
        if (!prefab)
        {
            Module::LogWarning("[AssetUpdater] assetUpdatePrefab_ が未設定なので、配信の更新は確認しません");
            return;
        }

        if (const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock())
            assetUpdate_ = ui->Components().Catch<AssetUpdatePresenter>();
    }

    void TitleScreenPresenter::OnUpdate()
    {
        if (!view_)
            return;

        using UiFlow::UiAction;
        auto& input = screen_->Input();

        switch (phase_)
        {
        case Phase::Press:
            if (!input.IsAnyPressed())
                break;
            
            if (!view_->IsIntroFinished())
            {
                view_->SkipIntro();
                break;
            }
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);
            view_->ShowMenu();
            phase_ = Phase::Menu;
            break;

        case Phase::Menu:
            if (!view_->IsMenuReady())
                break;
            if (input.IsPressed(UiAction::Up))
                Select(selection_ - 1);
            else if (input.IsPressed(UiAction::Down))
                Select(selection_ + 1);
            else if (input.IsPressed(UiAction::Submit))
                Decide(selection_);
            else if (input.IsPressed(UiAction::Cancel))
            {
                Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cancel);
                view_->HideMenu();
                phase_ = Phase::Press;
            }
            break;

        case Phase::Settings:
        case Phase::Leaving:
            break;
        }
    }

    void TitleScreenPresenter::Select(const int index)
    {
        const int clamped = std::clamp(index, 0, TitleScreenUi::MENU_COUNT - 1);
        if (clamped != selection_)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        selection_ = clamped;
        view_->SetSelection(selection_);
    }

    void TitleScreenPresenter::Decide(const int index)
    {
        Select(index);
        switch (selection_)
        {
        case TitleScreenUi::START_INDEX:
            StartGame();
            break;
        case TitleScreenUi::SETTINGS_INDEX:
            OpenSettings();
            break;
        default:
            ExitGame();
            break;
        }
    }

    void TitleScreenPresenter::OpenSettings()
    {
        const auto prefab = settingsPrefab_.get();
        if (!prefab)
        {
            Module::LogWarning("[Title] settingsPrefab_ が未設定なので、設定画面を開けません");
            return;
        }

        if (SettingsScreenPresenter::Open(*prefab).expired())
            return;

        phase_ = Phase::Settings;
        view_->SetCovered(true);
    }

    void TitleScreenPresenter::StartGame()
    {
        if (const auto assetUpdate = assetUpdate_.lock(); assetUpdate && !assetUpdate->TryStartGame())
            return;

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::GameStart);
        phase_ = Phase::Leaving;
        switch (GameCore::LoadGameProgression())
        {
        case GameCore::GameProgresion::FirstTouchDownMainIsLand:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::FirstTouchDownMainIsLand);
            break;
        case GameCore::GameProgresion::MainIsland:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::MainIsland);
            break;
        default:
            Module::LogError("Gameの進行状況に応じたScene遷移が定義されていません。");
            phase_ = Phase::Menu;
            break;
        }
    }

    void TitleScreenPresenter::ExitGame()
    {
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        phase_ = Phase::Leaving;
        Core::Application::ApplicationBase::RequestClose();
    }

    void TitleScreenPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("assetUpdatePrefab_", assetUpdatePrefab_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("settingsPrefab_", settingsPrefab_);
        ImGui::Text("phase: %d  selection: %d", static_cast<int>(phase_), selection_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::TitleScreenPresenter);
#pragma endregion
