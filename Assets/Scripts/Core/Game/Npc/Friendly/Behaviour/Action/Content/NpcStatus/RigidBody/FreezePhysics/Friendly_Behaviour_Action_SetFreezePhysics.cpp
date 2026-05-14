#include "Friendly_Behaviour_Action_SetFreezePhysics.h"

#include "../../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetFreezePhysics::DoTick(
        const TickContext& context)
    {
        context.NpcCollider().SetFreezePhysics(constraints_);
        return TickStatus::Success;
    }

    void Action::SetFreezePhysics::DoDrawGui()
    {
        Physics::DrawConstraintCheckBoxsGui(constraints_);
    }
}
