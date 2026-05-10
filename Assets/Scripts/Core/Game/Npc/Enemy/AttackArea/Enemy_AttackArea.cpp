#include "Enemy_AttackArea.h"

namespace GameCore::Npc::Enemy
{
    void AttackArea::DoAttack(AttackTarget attackTarget, std::unique_ptr<IDamage> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }
}
