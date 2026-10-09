#include "BillBoardNpcChatIcon.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void BillBoardNpcChatIcon::Show(
        const bool chattableIcon,
        const bool chattingIcon,
        const bool surpriseIcon)
    {
        isShow_             = true;
        hasRequested_       = true;
        requestedChattable_ = chattableIcon;
        requestedChatting_  = chattingIcon;
        requestedSurprise_  = surpriseIcon;
        Apply();
    }

    void BillBoardNpcChatIcon::Hide()
    {
        isShow_             = false;
        hasRequested_       = true;
        requestedChattable_ = false;
        requestedChatting_  = false;
        requestedSurprise_  = false;
        Apply();
    }

    void BillBoardNpcChatIcon::OnChattable()
    {
        if (!isShow_)
            return;

        CaptureRequestedIfNeeded();
        requestedChattable_ = false;
        requestedChatting_  = true;
        Apply();
    }

    void BillBoardNpcChatIcon::OnExitChattable()
    {
        if (!isShow_)
            return;

        CaptureRequestedIfNeeded();
        requestedChattable_ = true;
        requestedChatting_  = false;
        Apply();
    }

    void BillBoardNpcChatIcon::BeginReactionSurprise()
    {
        if (isReactionSurprise_)
            return;

        CaptureRequestedIfNeeded();
        isReactionSurprise_ = true;

        if (chattableIcon_) chattableIcon_->SetEnable(false);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(false);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(true);
    }

    void BillBoardNpcChatIcon::EndReactionSurprise()
    {
        if (!isReactionSurprise_)
            return;

        isReactionSurprise_ = false;
        Apply();
    }

    void BillBoardNpcChatIcon::SetObjectiveSurprise(const bool enable)
    {
        if (isObjectiveSurprise_ == enable)
            return;

        CaptureRequestedIfNeeded();
        isObjectiveSurprise_ = enable;
        Apply();
    }

    void BillBoardNpcChatIcon::CaptureRequestedIfNeeded()
    {
        if (hasRequested_ || isReactionSurprise_)
            return;

        hasRequested_       = true;
        requestedChattable_ = chattableIcon_ && chattableIcon_->IsEnable();
        requestedChatting_  = chattingIcon_  && chattingIcon_ ->IsEnable();
        requestedSurprise_  = surpriseIcon_  && surpriseIcon_ ->IsEnable();
    }

    void BillBoardNpcChatIcon::Apply()
    {
        // NOTE: リアクションの間はリアクションが見た目を持つ。終わったら頼まれた状態に戻す
        if (isReactionSurprise_)
            return;

        // NOTE: 目的の相手は「話せる」の代わりに驚きアイコンを出す。話しかけられる距離の表示(chatting)はそのまま
        const bool objective = isObjectiveSurprise_ && isShow_;
        if (chattableIcon_) chattableIcon_->SetEnable(requestedChattable_ && !objective);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(requestedChatting_);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(requestedSurprise_ || objective);
    }

    void BillBoardNpcChatIcon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chattableIcon_", chattableIcon_);
        ImGuiHelper::OnDrawInputField("chattingIcon_", chattingIcon_);
        ImGuiHelper::OnDrawInputField("surpriseIcon_", surpriseIcon_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BillBoardNpcChatIcon);
#pragma endregion
