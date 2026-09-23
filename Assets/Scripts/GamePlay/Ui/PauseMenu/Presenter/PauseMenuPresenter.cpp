#include "PauseMenuPresenter.h"

#include "DxLib.h"

#include "../Ui_PauseMenu.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/PlayerAvatar/Input/PlayerAvatarInput_void.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/State/Transition/SwordManAvatarStateTransition.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarControlAcceptance;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateAction;

        // 左スティックを方向キーとして読むためのしきい値
        constexpr short PAUSE_MENU_STICK_DEADZONE = 12000;
    }

    class PauseMenuPresenter::OpenMenuDeclaration final : public GameCore::PlayerAvatar::SwordMan::ISwordManAvatarTransitionVisitor
    {
    public:
        [[nodiscard]] bool IsDeclared() const { return isDeclared_; }

        void Action(const SwordManAvatarStateAction action, const bool isUsable) override
        {
            if (action == SwordManAvatarStateAction::OpenMenu && isUsable)
                isDeclared_ = true;
        }

    private:
        bool isDeclared_ = false;
    };

    void PauseMenuPresenter::Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar)
    {
        swordManAvatar_ = swordManAvatar;

        view_ = RequireComponent<PauseMenuUi>();
        view_->BuildRows();
        view_->ShowCharacter();
        view_->SetVisible(false);
    }

    void PauseMenuPresenter::OnUpdate()
    {
        const auto avatar = swordManAvatar_.lock();
        if (!avatar || !view_)
            return;

        if (isResumePending_)
        {
            isResumePending_ = false;
            avatar->EnableStateMachiine();
        }

        if (isOpen_)
        {
            UpdateOpened(*avatar);
            return;
        }

        if (avatar->GetInputAction().OpenMenu().IsPressed() && IsOpenMenuDeclared(*avatar))
            Open(*avatar);
    }

    void PauseMenuPresenter::UpdateOpened(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar)
    {
        const Keys keys = ReadKeys();
        const bool isToggled = avatar.GetInputAction().OpenMenu().IsPressed();

        const std::size_t previousIndex = model_.SelectedIndex();
        if (keys.prev && !previousKeys_.prev)
            model_.MoveSelection(-1);
        if (keys.next && !previousKeys_.next)
            model_.MoveSelection(1);
        if (model_.SelectedIndex() != previousIndex)
            view_->HighlightRow(model_.SelectedIndex());

        if (isToggled || (keys.cancel && !previousKeys_.cancel))
            Close();
        else if (keys.confirm && !previousKeys_.confirm)
            Confirm();

        previousKeys_ = keys;

        if (isOpen_)
            view_->Present(avatar.PlayerStatus());
    }

    void PauseMenuPresenter::Open(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar)
    {
        isOpen_ = true;
        // 開いた瞬間に押しっぱなしのキーを、決定や移動として拾わない
        previousKeys_ = ReadKeys();
        model_.Reset();

        avatar.DisableStateMachine();

        // book_ ごと有効に戻すと全行の印が出るので、強調は必ず当て直す
        view_->SetVisible(true);
        view_->HighlightRow(model_.SelectedIndex());
        view_->Present(avatar.PlayerStatus());
    }

    void PauseMenuPresenter::Close()
    {
        isOpen_ = false;
        isResumePending_ = true;
        view_->SetVisible(false);
    }

    void PauseMenuPresenter::Confirm()
    {
        switch (model_.Selected())
        {
        case PauseMenuEntry::Resume:
            Close();
            return;
        case PauseMenuEntry::ReturnToTitle:
            Close();
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::Title);
            return;
        // 各項目の頁はまだ無いので、右の頁はステータスのまま
        case PauseMenuEntry::Status:
        case PauseMenuEntry::Items:
        case PauseMenuEntry::Quests:
        case PauseMenuEntry::Controls:
            return;
        }
    }

    PauseMenuPresenter::Keys PauseMenuPresenter::ReadKeys()
    {
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);

        return Keys{
            .prev    = CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_UP] || xInput.ThumbLY > PAUSE_MENU_STICK_DEADZONE,
            .next    = CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_DOWN] || xInput.ThumbLY < -PAUSE_MENU_STICK_DEADZONE,
            .confirm = CheckHitKey(KEY_INPUT_RETURN) || xInput.Buttons[XINPUT_BUTTON_A],
            .cancel  = CheckHitKey(KEY_INPUT_ESCAPE) || xInput.Buttons[XINPUT_BUTTON_B],
        };
    }

    bool PauseMenuPresenter::IsOpenMenuDeclared(const GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar)
    {
        const auto state = avatar.GetStateMachine().CurrentStateValue();
        if (!state || state->ControlAcceptance() != PlayerAvatarControlAcceptance::Accept)
            return false;

        OpenMenuDeclaration declaration;
        state->VisitTransitions(declaration);
        return declaration.IsDeclared();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PauseMenuPresenter);
#pragma endregion
