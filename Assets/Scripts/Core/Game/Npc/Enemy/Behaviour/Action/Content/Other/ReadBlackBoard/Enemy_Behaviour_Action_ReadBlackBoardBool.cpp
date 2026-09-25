#include "Enemy_Behaviour_Action_ReadBlackBoardBool.h"

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    TickStatus ReadBlackBoardBool::DoTick(const TickContext& context)
    {
        const auto paramBool = context.Parameter()->Catch<bool>(keyName_);
        if (!paramBool) return TickStatus::Failure;

        if (paramBool->Get() == equalValue_)
            return TickStatus::Success;

        return TickStatus::Failure;
    }

    void ReadBlackBoardBool::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("equalValue_", equalValue_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ReadBlackBoardBool, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
