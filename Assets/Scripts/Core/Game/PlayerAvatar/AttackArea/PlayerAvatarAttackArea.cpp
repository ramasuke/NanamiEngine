#include "PlayerAvatarAttackArea.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar
{
    void PlayerAttackArea::DoAttack(
        AttackTarget attackTarget,
        std::unique_ptr<IDamage> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }
}

#pragma region SerializationMacro
REGISTER_ATTACK_AREA_TYPE(GameCore::PlayerAvatar::ITakablePlayerAttack)
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::PlayerAttackArea);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::AttackArea<GameCore::PlayerAvatar::ITakablePlayerAttack>, GameCore::PlayerAvatar::PlayerAttackArea);
#pragma endregion
