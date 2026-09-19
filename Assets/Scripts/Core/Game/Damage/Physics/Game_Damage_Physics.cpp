#include "Game_Damage_Physics.h"

#include "ext/quaternion_geometric.hpp"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore
{
    Damage::Physics::Physics(
        GameObject::IGameObject& from,
        GameObject::IGameObject& to,
        const PhysicsPower damageValue,
        const std::weak_ptr<GameObject::IGameObject>& hitPart,
        const bool isChargedAttack)
        : damageDirection_(from.Transform().GetWorldPos() - to.Transform().GetWorldPos())
        , damageValue_(damageValue)
        , hitPart_(hitPart)
        , isChargedAttack_(isChargedAttack)
    {

    }

    int Damage::Physics::DamageValue()
    {
        return damageValue_.Value();
    }

    std::weak_ptr<GameObject::IGameObject> Damage::Physics::HitPart() const
    {
        return hitPart_;
    }

    bool Damage::Physics::IsChargedAttack() const
    {
        return isChargedAttack_;
    }

    glm::vec3 Damage::Physics::DamageDirection() const
    {
        // damageDirection_ は「target から見た attacker 方向」。ノックバックは attacker から離れる向きにしたいので符号反転する。
        return glm::normalize(-damageDirection_);
    }
}
