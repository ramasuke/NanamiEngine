#include "GamePlay_Enemy_IAttackProjectile.h"

#include "../../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"

namespace GamePlay::Npc::Enemy
{
    void SetProjectileDamage(const std::weak_ptr<GameObject::IGameObject>& projectile, const GameCore::Damage::PhysicsPower power)
    {
        const auto projectileObject = projectile.lock();
        if (!projectileObject)
            return;

        if (const auto attackProjectile = projectileObject->Components().Catch<IAttackProjectile>().lock())
            attackProjectile->SetDamage(power);
    }
}
