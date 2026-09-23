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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::AttackArea);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::AttackArea<GameCore::Npc::Enemy::ITakableEnemyAttack>, GameCore::Npc::Enemy::AttackArea);
#pragma endregion
