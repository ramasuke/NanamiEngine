#pragma once
#include <cstdint>
#include "vec3.hpp"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GameCore::PlayerAvatar
{
    // NOTE: 調べたときに何をする対象か。操作ガイドの文言を選ぶのに使う
    enum class PlayerInteractKind : std::uint8_t
    {
        Talk,
        Open,
        Gather,
        Read,
        Board,
    };

    class IPlayerInteractable
    {
    public:
        virtual ~IPlayerInteractable() = default;
        virtual void OnInteractable() = 0;
        virtual void OnExitInteractable() = 0;
        virtual void OnInteract() = 0;
        // NOTE: false の間は範囲内にいても調べる対象にならない
        [[nodiscard]] virtual bool CanInteract() const { return true; }
        [[nodiscard]] virtual PlayerInteractKind InteractKind() const { return PlayerInteractKind::Talk; }
        [[nodiscard]] virtual const NanamiEngine::Module::GameObject::Transform& InteractableTransform() const = 0;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::IPlayerInteractable, 0)