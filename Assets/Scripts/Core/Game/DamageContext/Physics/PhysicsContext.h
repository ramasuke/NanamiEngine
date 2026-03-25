#pragma once
#include "PhysicsPower.h"
#include "vec3.hpp"
#include "../IDamageContext.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Damage
{
    struct PhysicsContext final : IDamageContext 
    {
        PhysicsContext(GameObject::IGameObject& from, GameObject::IGameObject& to, PhysicsPower damageValue);
        int DamageValue() override;

    private:
        glm::vec3 damageDirection_;
        PhysicsPower damageValue_;
    };
}
