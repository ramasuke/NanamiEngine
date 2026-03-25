#include "PhysicsContext.h"

#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore
{
    Damage::PhysicsContext::PhysicsContext(GameObject::IGameObject& from, GameObject::IGameObject& to, const PhysicsPower damageValue)
        : damageDirection_(from.TransformRef().GetWorldPos() - to.TransformRef().GetWorldPos())
        , damageValue_(damageValue)
    {
        
    }

    int Damage::PhysicsContext::DamageValue()
    {
        return damageValue_.Value();
    }
}
