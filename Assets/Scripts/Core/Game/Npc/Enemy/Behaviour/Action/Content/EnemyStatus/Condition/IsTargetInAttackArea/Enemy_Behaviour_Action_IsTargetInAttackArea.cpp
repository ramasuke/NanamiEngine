#include "Enemy_Behaviour_Action_IsTargetInAttackArea.h"

#include <algorithm>

#include "../../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../AttackArea/Enemy_AttackArea.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsTargetInAttackArea::DoTick(const TickContext& context)
    {
        const auto& attackArea = context.CatchPrefabObject<AttackArea>(attackAreaName_);
        // 壊せる小物も ITakableEnemyAttack として入ってくるので、プレイヤーだけを見る
        const bool hasTarget = std::ranges::any_of(attackArea.Targets(), [](const AttackArea::AttackTarget& target)
        {
            return !target.GameObject().Components().Catch<IPlayerAvatar>().expired();
        });
        return (isInner_ == hasTarget) ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::IsTargetInAttackArea::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("attackAreaName_", attackAreaName_);
        ImGuiHelper::OnDrawInputField("isInner_", isInner_);
    }
}
