#include "Enemy_Behaviour_Action_SetLinearVelocity.h"

#include "../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::SetLinearVelocity::DoTick(const TickContext& context)
    {
        if (isGravity_)
        {
            velocity_.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y;
        }
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), velocity_);
        return TickStatus::Success;
    }

    void Action::SetLinearVelocity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("velocity_", velocity_);
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
    }
}
