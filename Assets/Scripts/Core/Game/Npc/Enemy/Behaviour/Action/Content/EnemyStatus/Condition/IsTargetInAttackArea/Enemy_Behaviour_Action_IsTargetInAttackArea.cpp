#include "Enemy_Behaviour_Action_IsTargetInAttackArea.h"
#include "../../../../../../AttackArea/Enemy_AttackArea.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsTargetInAttackArea::DoTick(const TickContext& context)
    {
        const auto& attackArea = context.CatchPrefabObject<AttackArea>(attackAreaName_);
        const bool hasTarget = !attackArea.Targets().empty();
        return (isInner_ == hasTarget) ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::IsTargetInAttackArea::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("attackAreaName_", attackAreaName_);
        ImGuiHelper::OnDrawInputField("isInner_", isInner_);
    }
}
