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
        Physics(GameObject::IGameObject& from,
                GameObject::IGameObject& to,
                PhysicsPower damageValue,
                const std::weak_ptr<GameObject::IGameObject>& hitPart,
                bool isChargedAttack);
        int DamageValue() override;
        [[nodiscard]] glm::vec3 DamageDirection() const override;
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> HitPart() const override;
        [[nodiscard]] bool IsChargedAttack() const override;

    private:
        glm::vec3 damageDirection_;
        PhysicsPower damageValue_;
        std::weak_ptr<GameObject::IGameObject> hitPart_;
        bool isChargedAttack_;
    };
}
