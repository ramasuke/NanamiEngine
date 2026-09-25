#include "Enemy_Behaviour_Action_WriteBlackBoardBool.h"

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::WriteBlackBoardBool, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
