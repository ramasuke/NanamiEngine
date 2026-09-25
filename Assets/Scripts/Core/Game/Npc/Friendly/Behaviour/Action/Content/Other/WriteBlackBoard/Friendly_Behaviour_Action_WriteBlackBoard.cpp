#include "Friendly_Behaviour_Action_WriteBlackBoard.h"

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::WriteBlackBoard, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
