#pragma once
#include "vec3.hpp"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerChattable
    {
    public:
        virtual ~IPlayerChattable() = default;
        virtual void OnChattable() = 0;
        virtual void OnExitChattable() = 0;
        virtual void OnChat() = 0;
        [[nodiscard]] virtual const NanamiEngine::Module::GameObject::Transform& ChattableTransform() const = 0;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::IPlayerChattable, 0)