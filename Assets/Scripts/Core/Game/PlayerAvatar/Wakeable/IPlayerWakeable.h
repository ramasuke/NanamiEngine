#pragma once
#include "vec3.hpp"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerWakeable
    {
    public:
        virtual ~IPlayerWakeable() = default;
        virtual void OnEnterWakeUpRange() = 0;
        virtual void OnExitWakeUpRange() = 0;
        virtual void RequestWakeUp() = 0;
        [[nodiscard]] virtual bool IsDowned() const = 0;
        [[nodiscard]] virtual const NanamiEngine::Module::GameObject::Transform& WakeableTransform() const = 0;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::IPlayerWakeable, 0)
