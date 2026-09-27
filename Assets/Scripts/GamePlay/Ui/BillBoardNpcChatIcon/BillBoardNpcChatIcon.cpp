#include "BillBoardNpcChatIcon.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void BillBoardNpcChatIcon::Show(
        const bool chattableIcon,
        const bool chattingIcon,
        const bool surpriseIcon)
    {
        isShow_ = true;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, chattableIcon);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , chattingIcon);
        SetIconEnable(surpriseIcon_ .get().get(), savedSurprise_ , surpriseIcon);
    }

    void BillBoardNpcChatIcon::Hide()
    {
        isShow_ = false;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, false);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , false);
        SetIconEnable(surpriseIcon_ .get().get(), savedSurprise_ , false);
    }

    void BillBoardNpcChatIcon::OnChattable()
    {
        if (!isShow_)
            return;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, false);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , true);
    }

    void BillBoardNpcChatIcon::OnExitChattable()
    {
        if (!isShow_)
            return;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, true);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , false);
    }

    void BillBoardNpcChatIcon::BeginReactionSurprise()
    {
        if (isReactionSurprise_)
            return;

        savedChattable_ = chattableIcon_ && chattableIcon_->IsEnable();
        savedChatting_  = chattingIcon_  && chattingIcon_ ->IsEnable();
        savedSurprise_  = surpriseIcon_  && surpriseIcon_ ->IsEnable();
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
        if (chattableIcon_) chattableIcon_->SetEnable(savedChattable_);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(savedChatting_);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(savedSurprise_);
    }

    void BillBoardNpcChatIcon::SetIconEnable(GameObject::IGameObject* icon, bool& reactionSaved, const bool enable) const
    {
        if (!icon)
            return;

        if (isReactionSurprise_)
            reactionSaved = enable;
        else
            icon->SetEnable(enable);
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
