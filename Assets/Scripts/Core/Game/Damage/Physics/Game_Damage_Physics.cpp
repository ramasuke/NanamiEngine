#include "Game_Damage_Physics.h"

#include "ext/quaternion_geometric.hpp"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore
{
    Damage::Physics::Physics(GameObject::IGameObject& from, GameObject::IGameObject& to, const PhysicsPower damageValue)
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
        // damageDirection_ は「target から見た attacker 方向」。ノックバックは attacker から離れる向きにしたいので符号反転する。
        return glm::normalize(-damageDirection_);
    }
}
