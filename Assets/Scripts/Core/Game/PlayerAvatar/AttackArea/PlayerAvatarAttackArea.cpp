#include "PlayerAvatarAttackArea.h"

namespace GameCore::PlayerAvatar
{
    void PlayerAttackArea::DoAttack(
        AttackTarget attackTarget,
        std::unique_ptr<IDamage> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }
}
