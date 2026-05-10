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

REGISTER_ATTACK_AREA_TYPE(GameCore::PlayerAvatar::ITakablePlayerAttack)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::PlayerAttackArea, 1);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::PlayerAttackArea);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::AttackArea<GameCore::PlayerAvatar::ITakablePlayerAttack>, GameCore::PlayerAvatar::PlayerAttackArea);
#pragma endregion
