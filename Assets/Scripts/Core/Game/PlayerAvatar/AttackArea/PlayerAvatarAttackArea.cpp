#include "PlayerAvatarAttackArea.h"

namespace GameCore::PlayerAvatar
{
    void PlayerAttackArea::DoAttack(AttackTarget attackTarget, std::unique_ptr<IDamageContext> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }
}
