#include "CharacterSelectPresenter.h"

#include <limits>


#include "../UI_CharacterSelect.h"
#include "../../../Prop/CharacterPodium/Prop_CharacterPodium.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void CharacterSelectPresenter::Bind(const std::weak_ptr<Prop::CharacterPodium>& podium)
    {
        podium_ = podium;
        if (hasStarted_)
            Open();
    }

    void CharacterSelectPresenter::OnStart()
    {
        hasStarted_ = true;
        if (!podium_.expired())
            Open();
    }

    void CharacterSelectPresenter::Open()
    {
        if (isClosed_ || model_)
            return;

        const auto podium = podium_.lock();
        if (!podium || podium->Characters().empty())
        {
            NanamiEngine::Module::LogError("CharacterSelectPresenter: 展示台が無いか名簿が空なので、キャラ選択を開けません");
            Discard();
            return;
        }

        screen_ = RequireComponent<UiFlow::UiScreen>();
        // 会話のたびに二重に生えるのを防ぐ
        if (!screen_->Open())
        {
            Discard();
            return;
        }
        screen_->Input().Map().AddKey(UiFlow::UiAction::Submit, Platform::Input::Key::Space);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);

        view_  = RequireComponent<CharacterSelectUi>();
        model_ = std::make_unique<CharacterSelectModel>(podium->Characters());

        view_->BuildRoster(model_->Characters());

        const auto& rows = view_->Rows();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (const auto row = rows[i].lock())
            {
                row->SubscribeOnClickSelectButton([this, i]
                {
                    model_->Select(i);
                });
            }
        }

        model_->OnSelectionChanged().Subscribe([this](const size_t index)
        {
            view_->HighlightRow(index);
            if (const auto character = model_->Selected())
                view_->ShowDetail(*character);
            if (const auto current = podium_.lock())
                current->ShowCharacter(index);
        }).AddTo(this);

        // 今いるキャラに合わせて開く
        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;
        if (owner)
        {
            const auto& characters = model_->Characters();
            for (size_t i = 0; i < characters.size(); ++i)
            {
                if (characters[i]->AvatarType() == owner->Type())
                {
                    model_->Select(i);
                    break;
                }
            }
        }

        // Select は同じ index だと通知を出さないので、初期表示はここで一度だけ作る
        view_->HighlightRow(model_->SelectedIndex());
        if (const auto character = model_->Selected())
            view_->ShowDetail(*character);
        podium->ShowCharacter(model_->SelectedIndex());
        podium->FocusCamera();
    }

    void CharacterSelectPresenter::OnUpdate()
    {
        if (isClosed_)
            return;

        if (!model_)
        {
            NanamiEngine::Module::LogError("CharacterSelectPresenter: 展示台が Bind されていないので、キャラ選択を開けません");
            Discard();
            return;
        }

        using UiFlow::UiAction;
        auto& input = screen_->Input();

        const size_t previousIndex = model_->SelectedIndex();
        if (input.IsPressed(UiAction::Up) || input.IsPressed(UiAction::Left))
            model_->MoveSelection(-1);
        if (input.IsPressed(UiAction::Down) || input.IsPressed(UiAction::Right))
            model_->MoveSelection(1);
        // NOTE: 開いたときの初期選択でも OnSelectionChanged が来るので、音はキー操作でだけ鳴らす (マウスはホバーで鳴る)
        if (model_->SelectedIndex() != previousIndex)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        if (input.IsPressed(UiAction::Submit))
            Confirm();
        else if (input.IsPressed(UiAction::Cancel))
            Close(false);
    }

    void CharacterSelectPresenter::Confirm()
    {
        if (!model_->CanConfirm())
        {
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Refuse);
            return;
        }

        const auto character = model_->Selected();
        const auto owner = suspendedAvatar_.lock();
        if (owner && owner->Type() == character->AvatarType())
        {
            Close(false);
            return;
        }

        const auto scene = GameCore::Game::Instance().Scenes()
            .Catch<GameCore::Scene::Main::MainIslandScene>(GameCore::Scene::Main::SceneType::MainIsland);
        if (!scene)
        {
            Close(false);
            return;
        }

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Stamp);
        scene->SwitchPlayerAvatar(character->AvatarType());
        Close(true);
    }

    void CharacterSelectPresenter::Close(const bool didSwitch)
    {
        if (isClosed_)
            return;
        isClosed_ = true;

        if (!didSwitch)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);

        if (const auto podium = podium_.lock())
        {
            podium->ShowCharacter(std::numeric_limits<size_t>::max());
            podium->RestoreCamera();
        }

        screen_->Close();
        Entity().lock()->OnDestroy();
    }

    void CharacterSelectPresenter::Discard()
    {
        isClosed_ = true;
        Entity().lock()->OnDestroy();
    }

    void CharacterSelectPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::CharacterSelectPresenter);
#pragma endregion
