#include "PhysicsContext.h"

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
}
