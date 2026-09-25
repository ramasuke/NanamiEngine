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
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeIsGravity, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
