#include "Friendly_Behaviour_Action_ReadBlackBoard.h"

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::ReadBlackBoard, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
