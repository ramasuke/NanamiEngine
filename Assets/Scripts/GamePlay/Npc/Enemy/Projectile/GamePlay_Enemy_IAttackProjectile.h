#pragma once
#include "../../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"

namespace GamePlay::Npc::Enemy
{
    class IAttackProjectile
    {
    public:
        virtual ~IAttackProjectile() = default;
        virtual void SetDamage(GameCore::Damage::PhysicsPower power) = 0;
    };
}
