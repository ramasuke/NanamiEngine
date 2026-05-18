#include "Friendly_Behaviour_Action_SetEnableChatIcon.h"

#include "../../../../../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../../../../GamePlay/Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"

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
