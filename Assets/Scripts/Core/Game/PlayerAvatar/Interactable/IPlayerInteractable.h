#pragma once
#include "vec3.hpp"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerInteractable
    {
    public:
        virtual ~IPlayerInteractable() = default;
        virtual void OnInteractable() = 0;
        virtual void OnExitInteractable() = 0;
        virtual void OnInteract() = 0;
        /** false の間は範囲内にいても調べる対象にならない */
        [[nodiscard]] virtual bool CanInteract() const { return true; }
        [[nodiscard]] virtual const NanamiEngine::Module::GameObject::Transform& InteractableTransform() const = 0;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::IPlayerInteractable, 0)