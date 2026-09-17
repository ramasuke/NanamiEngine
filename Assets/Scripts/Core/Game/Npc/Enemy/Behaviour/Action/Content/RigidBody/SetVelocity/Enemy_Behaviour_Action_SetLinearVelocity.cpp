#include "Enemy_Behaviour_Action_SetLinearVelocity.h"

#include "../../../../../../../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::SetLinearVelocity::DoTick(const TickContext& context)
    {
        if (isGravity_)
        {
            velocity_.y = context.EnemyRigidBody().LinearVelocity().y;
        }
        context.EnemyRigidBody().SetLinearVelocity(velocity_);
        return TickStatus::Success;
    }

    void Action::SetLinearVelocity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("velocity_", velocity_);
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
    }
}
