#include "Friendly_Behaviour_Action_SetEnableChatIcon.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../../../../GamePlay/Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetEnableShowChatIcon::DoTick(
        const TickContext& context)
    {
        if (isShow_)
        {
            context.OwnChatIcon().Show(
                chattableIcon_,
                chattingIcon_,
                surpriseIcon_);
        }
        else
        {
            context.OwnChatIcon().Hide();
        }
        return TickStatus::Success;
    }

    void Action::SetEnableShowChatIcon::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isShow_", isShow_);
        ImGuiHelper::OnDrawInputField("chattableIcon_", chattableIcon_);
        ImGuiHelper::OnDrawInputField("chattingIcon_", chattingIcon_);
        ImGuiHelper::OnDrawInputField("surpriseIcon_", surpriseIcon_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetEnableShowChatIcon)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::SetEnableShowChatIcon)
#pragma endregion
