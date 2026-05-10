#pragma once
#include <memory>

#include "../../DamageContext/IDamageContext.h"

namespace GameCore::PlayerAvatar
{
    class ITakablePlayerAttack
    {
    public:
        virtual ~ITakablePlayerAttack() = default;
        virtual void OnTakeDamage(std::unique_ptr<IDamage> damage) = 0;
    };
}
