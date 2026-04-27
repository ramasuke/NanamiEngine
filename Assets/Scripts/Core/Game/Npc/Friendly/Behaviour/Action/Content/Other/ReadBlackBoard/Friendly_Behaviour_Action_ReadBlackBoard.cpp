#include "Friendly_Behaviour_Action_ReadBlackBoard.h"

#include "../../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::ReadBlackBoard::DoTick(const TickContext& context)
    {
        if (context.Parameter()->Catch<int>(keyName_)->Get() == equalValue_)
            return TickStatus::Success;
        
        return TickStatus::Failure;
    }

    void Action::ReadBlackBoard::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("equalValue_", equalValue_);
    }
}
