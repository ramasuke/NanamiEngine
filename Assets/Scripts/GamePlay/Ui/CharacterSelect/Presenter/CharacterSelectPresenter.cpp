#include "CharacterSelectPresenter.h"

#include <limits>

#include "DxLib.h"

#include "../UI_CharacterSelect.h"
#include "../../../Prop/CharacterPodium/Prop_CharacterPodium.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short CHARACTER_SELECT_STICK_DEADZONE = 12000;
    }

    bool CharacterSelectPresenter::isOpen_ = false;

    void CharacterSelectPresenter::Bind(const std::weak_ptr<Prop::CharacterPodium>& podium)
    {
        podium_ = podium;
    }

    void CharacterSelectPresenter::OnStart()
    {
        const auto podium = podium_.lock();
        if (isOpen_ || !podium || podium->Characters().empty())
        {
            isClosed_ = true;
            Entity().lock()->OnDestroy();
            return;
        }
        isOpen_ = true;

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

        model_->OnSelectionChanged().subscribe([this](const size_t index)
        {
            view_->HighlightRow(index);
            if (const auto character = model_->Selected())
                view_->ShowDetail(*character);
            if (const auto current = podium_.lock())
                current->ShowCharacter(index);
        });

        // 今いるキャラに合わせて開く
        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;
        if (owner)
        {
            owner->DisableStateMachine();

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
        if (isClosed_ || !model_)
            return;

        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);

        const bool isPrevPressed = CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W)
            || CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A)
            || xInput.Buttons[XINPUT_BUTTON_DPAD_UP] || xInput.Buttons[XINPUT_BUTTON_DPAD_LEFT]
            || xInput.ThumbLY > CHARACTER_SELECT_STICK_DEADZONE;
        const bool isNextPressed = CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S)
            || CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)
            || xInput.Buttons[XINPUT_BUTTON_DPAD_DOWN] || xInput.Buttons[XINPUT_BUTTON_DPAD_RIGHT]
            || xInput.ThumbLY < -CHARACTER_SELECT_STICK_DEADZONE;
        const bool isConfirmPressed = CheckHitKey(KEY_INPUT_RETURN) || CheckHitKey(KEY_INPUT_SPACE)
            || xInput.Buttons[XINPUT_BUTTON_A];
        const bool isCancelPressed = CheckHitKey(KEY_INPUT_ESCAPE) || xInput.Buttons[XINPUT_BUTTON_B];

        if (isPrevPressed && !wasPrevPressed_)
            model_->MoveSelection(-1);
        if (isNextPressed && !wasNextPressed_)
            model_->MoveSelection(1);
        if (isConfirmPressed && !wasConfirmPressed_)
            Confirm();
        else if (isCancelPressed && !wasCancelPressed_)
            Close(false);

        wasPrevPressed_    = isPrevPressed;
        wasNextPressed_    = isNextPressed;
        wasConfirmPressed_ = isConfirmPressed;
        wasCancelPressed_  = isCancelPressed;
    }

    void CharacterSelectPresenter::Confirm()
    {
        if (!model_->CanConfirm())
            return;

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

        scene->SwitchPlayerAvatar(character->AvatarType());
        Close(true);
    }

    void CharacterSelectPresenter::Close(const bool didSwitch)
    {
        if (isClosed_)
            return;
        isClosed_ = true;

        if (!didSwitch)
        {
            if (const auto owner = suspendedAvatar_.lock())
                owner->EnableStateMachiine();
        }

        if (const auto podium = podium_.lock())
        {
            podium->ShowCharacter(std::numeric_limits<size_t>::max());
            podium->RestoreCamera();
        }

        Entity().lock()->OnDestroy();
    }

    void CharacterSelectPresenter::OnDestroy()
    {
        isOpen_ = false;
    }
}
