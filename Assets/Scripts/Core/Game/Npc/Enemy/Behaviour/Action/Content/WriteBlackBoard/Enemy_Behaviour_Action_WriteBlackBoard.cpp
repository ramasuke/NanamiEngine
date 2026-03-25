#include "Enemy_Behaviour_Action_WriteBlackBoard.h"

#include "../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    TickStatus WriteBlackBoard::DoTick(const TickContext& context)
    {
        context.Parameter()->Catch<int>(keyName_)->Set(value_);
        
        return TickStatus::Success;
    }

    void WriteBlackBoard::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("value_", value_);
    }
}
