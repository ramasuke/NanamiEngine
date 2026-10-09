#include "Game_Damage_Physics.h"

#include "ext/quaternion_geometric.hpp"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore
{
    Damage::Physics::Physics(
        GameObject::IGameObject& from,
        GameObject::IGameObject& to,
        const PhysicsPower damageValue)
        : damageDirection_(from.Transform().GetWorldPos() - to.Transform().GetWorldPos())
        , damageValue_(damageValue)
    {

    }

    int Damage::Physics::DamageValue()
    {
        return damageValue_.Value();
    }

    glm::vec3 Damage::Physics::DamageDirection() const
    {
        // NOTE: damageDirection_ は target から見た attacker の向き。ノックバックは離れる向きなので反転する
        return glm::normalize(-damageDirection_);
    }
}
