#include "Enemy_Behaviour_Action_ChangeIsGravity.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeIsGravity::DoTick(const TickContext& context)
    {
        context.EnemyRigidBody().SetGravity(isGravity_);
        return TickStatus::Success;
    }

    void Action::ChangeIsGravity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeIsGravity)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeIsGravity)
#pragma endregion
