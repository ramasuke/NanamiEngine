#include "Enemy_AttackArea.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy
{
    void AttackArea::DoAttack(AttackTarget attackTarget, std::unique_ptr<IDamage> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }
}

#pragma region SerializationMacro
REGISTER_ATTACK_AREA_TYPE(GameCore::Npc::Enemy::ITakableEnemyAttack)
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::AttackArea, GamePlay::AttackArea<GameCore::Npc::Enemy::ITakableEnemyAttack>);
#pragma endregion
