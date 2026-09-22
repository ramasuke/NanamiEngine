#pragma once
#include <cstdint>
#include <memory>

#include "cereal/cereal.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;

    // アイテムを使ったときに起きること1つ分。中身の値は ItemData の .meta に入る
    class IItemEffect
    {
    public:
        virtual ~IItemEffect() = default;
        /** @param user 使ったアバター。爆弾のように世界へ何かを置く効果が位置と向きを取る */
        virtual void Apply(IItemEffectTarget& target, const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user) const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Item::IItemEffect, 0)
