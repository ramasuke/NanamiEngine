#pragma once
#include "../../../../../GamePlay/AttackArea/AttackArea.h"
#include "../ITakableEnemyAttack/ITakableEnemyAttack.h"
#include "cereal/cereal.hpp"

namespace GameCore::Npc::Enemy
{
    class AttackArea final : public GamePlay::AttackArea<ITakableEnemyAttack>
    {
        void DoAttack(AttackTarget attackTarget, std::unique_ptr<IDamage> context) override;
    };
}

REGISTER_ATTACK_AREA_TYPE(GameCore::Npc::Enemy::ITakableEnemyAttack)
#pragma region SerializationMacro
// NOTE: 2 = 基底が NetworkComponent になった版(GamePlay::AttackArea の load が参照する)
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::AttackArea, 2);
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::AttackArea);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::AttackArea<GameCore::Npc::Enemy::ITakableEnemyAttack>, GameCore::Npc::Enemy::AttackArea);
#pragma endregion