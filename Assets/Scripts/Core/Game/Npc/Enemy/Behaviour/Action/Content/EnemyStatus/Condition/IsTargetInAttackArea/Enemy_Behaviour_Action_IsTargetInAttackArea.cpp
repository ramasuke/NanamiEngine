#include "Enemy_Behaviour_Action_IsTargetInAttackArea.h"

#include <algorithm>

#include "../../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../AttackArea/Enemy_AttackArea.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::IsTargetInAttackArea)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::IsTargetInAttackArea)
#pragma endregion
