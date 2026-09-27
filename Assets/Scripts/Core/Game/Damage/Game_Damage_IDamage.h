#pragma once
#include <memory>
#include "vec3.hpp"
#include "Flinch/Game_Damage_FlinchPower.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore
{
    struct IDamage
    {
    public:
        virtual ~IDamage() = default;
        virtual int DamageValue() = 0;
        
        [[nodiscard]] virtual glm::vec3 DamageDirection() const = 0;
        [[nodiscard]] virtual Damage::FlinchPower FlinchPower() const { return Damage::FlinchPower(); }
    };
}
