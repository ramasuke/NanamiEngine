#include "GamePlay_Enemy_ProjectileAttackArea.h"

#include <algorithm>

#include "../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../Core/Game/Damage/Physics/Game_Damage_Physics.h"
#include "../../../../Core/Game/Npc/Enemy/AttackArea/Enemy_AttackArea.h"
#include "../../../../Core/Game/Npc/Enemy/ITakableEnemyAttack/ITakableEnemyAttack.h"

namespace GamePlay::Npc::Enemy
{
    void AttackProjectile::SetDamage(const GameCore::Damage::PhysicsPower power)
    {
        power_ = power;
    }

    void AttackProjectile::OnTriggerEnter(
        const Physics::Manifold&,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        // ダメージ0の投射物は演出専用なので被弾リアクションも起こさない
        if (power_.Value() <= 0)
            return;

        const auto owner  = Physics::FindBodyOwner(gameObject);
        const auto target = owner->Components().Catch<GameCore::Npc::Enemy::ITakableEnemyAttack>().lock();
        if (!target)
            return;

        const bool isAlreadyHit = std::ranges::any_of(hitObjects_,
            [&](const std::weak_ptr<GameObject::IGameObject>& hit) { return hit.lock() == owner; });
        if (isAlreadyHit)
            return;

        if (!GameCore::Npc::Enemy::AttackArea::IsDamageApplicableTarget(*owner))
            return;

        const auto self = Entity().lock();
        if (!self)
            return;

        hitObjects_.emplace_back(owner);
        target->OnTakeDamage(std::make_unique<GameCore::Damage::Physics>(
            *self, *owner, power_, gameObject, false));
    }

    void AttackProjectile::OnDrawGui()
    {
        ImGui::Text("Damage: %d", power_.Value());
    }
}
