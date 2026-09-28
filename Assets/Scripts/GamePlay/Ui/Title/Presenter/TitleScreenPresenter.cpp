#include "TitleScreenPresenter.h"

#include <algorithm>

#include "Assets/Scripts/Core/Input/InputAliases.h"
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
    namespace
    {
        // 左スティックしきい値
        constexpr short TITLE_STICK_DEADZONE = 12000;
    }

    void TitleScreenPresenter::OnStart()
    {
        view_ = RequireComponent<TitleScreenUi>();
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
                if (phase_ == Phase::Menu && view_->IsMenuReady() && !IsAssetUpdatePrompting())
                {
                    Select(index);
                }
            }).AddTo(this);
            button->OnClick().Subscribe([this, index](NanamiUi::MouseState)
            {
                if (phase_ == Phase::Menu && view_->IsMenuReady() && !IsAssetUpdatePrompting())
                {
                    Decide(index);
                }
            }).AddTo(this);
        }

        // 起動前から押しっぱなしのキーでメニューを開かない
        previousKeys_ = ReadKeys();

        const auto prefab = assetUpdatePrefab_.get();
        if (!prefab)
        {
            Module::LogWarning("[AssetUpdater] assetUpdatePrefab_ が未設定なので、配信の更新は確認しません");
            return;
        }

        // UIは world 座標がそのままスクリーン座標
        if (const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock())
            assetUpdate_ = ui->Components().Catch<AssetUpdatePresenter>();
    }

    void TitleScreenPresenter::OnUpdate()
    {
        const Keys keys = ReadKeys();
        const Keys pressed{
            keys.any     && !previousKeys_.any,
            keys.up      && !previousKeys_.up,
            keys.down    && !previousKeys_.down,
            keys.confirm && !previousKeys_.confirm,
            keys.cancel  && !previousKeys_.cancel,
        };
        previousKeys_ = keys;

        if (!view_ || IsAssetUpdatePrompting())
            return;

        switch (phase_)
        {
        case Phase::Press:
            if (!pressed.any)
                break;
            
            // 出だしの途中で押されたら、まず出し切るだけにする
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
            if (pressed.up)
                Select(selection_ - 1);
            else if (pressed.down)
                Select(selection_ + 1);
            else if (pressed.confirm)
                Decide(selection_);
            else if (pressed.cancel)
            {
                Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cancel);
                view_->HideMenu();
                phase_ = Phase::Press;
            }
            break;

        case Phase::Settings:
            // NOTE: 設定を閉じた ESC / B でメニューまで畳まないよう、戻ったフレームは入力を見ない
            if (!SettingsScreenPresenter::IsOpen() || settings_.expired())
            {
                view_->SetCovered(false);
                phase_ = Phase::Menu;
            }
            break;

        case Phase::Leaving:
            break;
        }
    }

    TitleScreenPresenter::Keys TitleScreenPresenter::ReadKeys()
    {
        const auto xInput = Gamepad::Get();
        Keys keys;
        keys.any = NanamiEngine::Platform::Input::IsAnyDeviceDown() || xInput.IsAnyDown();
        keys.up = Keyboard::IsDown(Key::Up) || Keyboard::IsDown(Key::W)
            || xInput.IsDown(GamepadButton::DPadUp) || xInput.thumbLY > TITLE_STICK_DEADZONE;
        keys.down = Keyboard::IsDown(Key::Down) || Keyboard::IsDown(Key::S)
            || xInput.IsDown(GamepadButton::DPadDown) || xInput.thumbLY < -TITLE_STICK_DEADZONE;
        keys.confirm = Keyboard::IsDown(Key::Return) || Keyboard::IsDown(Key::Space)
            || xInput.IsDown(GamepadButton::A) || xInput.IsDown(GamepadButton::Start);
        keys.cancel = Keyboard::IsDown(Key::Escape) || Keyboard::IsDown(Key::Back)
            || xInput.IsDown(GamepadButton::B);
        return keys;
    }

    bool TitleScreenPresenter::IsAssetUpdatePrompting() const
    {
        const auto assetUpdate = assetUpdate_.lock();
        return assetUpdate && assetUpdate->IsPrompting();
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

        settings_ = SettingsScreenPresenter::Open(*prefab);
        if (settings_.expired())
            return;

        phase_ = Phase::Settings;
        view_->SetCovered(true);
    }

    void TitleScreenPresenter::StartGame()
    {
        // 更新が済んでいなければ荷札を出し直す。確認中・受け取り中の押下は受け流す
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
