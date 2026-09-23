#pragma once
#include "../../../../GamePlay/AttackArea/AttackArea.h"
#include "../ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace GameCore::PlayerAvatar
{
    class PlayerAttackArea final : public GamePlay::AttackArea<ITakablePlayerAttack>
    {
        void DoAttack(AttackTarget attackTarget, std::unique_ptr<IDamage> context) override;
    };
}

ATTACK_AREA_CLASS_VERSION(GameCore::PlayerAvatar::ITakablePlayerAttack)
#pragma region SerializationMacro
// NOTE: 2 = 基底が NetworkComponent になった版(GamePlay::AttackArea の load が参照する)
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::PlayerAttackArea, 2);
#pragma endregion
