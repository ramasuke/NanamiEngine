#pragma once
#include "Game_Damage_PhysicsPower.h"
#include "vec3.hpp"
#include "../Game_Damage_IDamage.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Damage
{
    struct Physics final : IDamage
    {
        Physics(GameObject::IGameObject& from, GameObject::IGameObject& to, PhysicsPower damageValue);
        int DamageValue() override;

    private:
        glm::vec3 damageDirection_;
        PhysicsPower damageValue_;
    };
}
