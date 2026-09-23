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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ReadBlackBoardBool)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::ReadBlackBoardBool
)
#pragma endregion
