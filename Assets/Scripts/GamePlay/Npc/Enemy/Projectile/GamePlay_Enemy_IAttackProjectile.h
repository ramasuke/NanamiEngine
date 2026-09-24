#pragma once
#include <memory>

#include "../../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Npc::Enemy
{
    class IAttackProjectile
    {
    public:
        virtual ~IAttackProjectile() = default;
        virtual void SetDamage(GameCore::Damage::PhysicsPower power) = 0;

        /** 生成済みの投射物が IAttackProjectile を持っていればダメージを設定する */
        static void TrySetDamage(const std::weak_ptr<GameObject::IGameObject>& projectile, GameCore::Damage::PhysicsPower power);
    };
}
