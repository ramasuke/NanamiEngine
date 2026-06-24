#include "GamePlay_Enemy_ProjectileAttackArea.h"

#include "../../../../Core/Game/Npc/Enemy/AttackArea/Enemy_AttackArea.h"

namespace GamePlay::Npc::Enemy
{
    void AttackProjectile::OnAwake()
    {
        RequireComponent<GameCore::Npc::Enemy::AttackArea>();
    }

    void AttackProjectile::SetDamage(const GameCore::Damage::PhysicsPower power)
    {
        power_ = power;
    }

    void AttackProjectile::OnDrawGui()
    {
    }
}
