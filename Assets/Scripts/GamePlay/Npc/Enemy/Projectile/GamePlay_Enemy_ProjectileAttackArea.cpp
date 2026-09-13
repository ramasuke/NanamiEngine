#include "GamePlay_Enemy_ProjectileAttackArea.h"

#include <algorithm>

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

        const auto target = gameObject->Components().Catch<GameCore::Npc::Enemy::ITakableEnemyAttack>().lock();
        if (!target)
            return;

        const bool isAlreadyHit = std::ranges::any_of(hitObjects_,
            [&](const std::weak_ptr<GameObject::IGameObject>& hit) { return hit.lock() == gameObject; });
        if (isAlreadyHit)
            return;

        if (!GameCore::Npc::Enemy::AttackArea::IsDamageApplicableTarget(*gameObject))
            return;

        const auto self = Entity().lock();
        if (!self)
            return;

        hitObjects_.emplace_back(gameObject);
        target->OnTakeDamage(std::make_unique<GameCore::Damage::Physics>(*self, *gameObject, power_));
    }

    void AttackProjectile::OnDrawGui()
    {
        ImGui::Text("Damage: %d", power_.Value());
    }
}
