#include "Enemy_Behaviour_Action_WriteBlackBoardBool.h"

#include "../../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    TickStatus WriteBlackBoardBool::DoTick(const TickContext& context)
    {
        context.Parameter()->Catch<bool>(keyName_)->Set(value_);
        
        return TickStatus::Success;
    }

    void WriteBlackBoardBool::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("value_", value_);
    }
}