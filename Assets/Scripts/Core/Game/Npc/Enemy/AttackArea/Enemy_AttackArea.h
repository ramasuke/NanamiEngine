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

ATTACK_AREA_CLASS_VERSION(GameCore::Npc::Enemy::ITakableEnemyAttack)
#pragma region SerializationMacro
// NOTE: 2 = 基底が NetworkComponent になった版(GamePlay::AttackArea の load が参照する)
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::AttackArea, 2);
#pragma endregion
