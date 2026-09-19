#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;

    // アイテムを使ったときに起きること1つ分。中身の値は ItemData の .meta に入る
    class IItemEffect
    {
    public:
        virtual ~IItemEffect() = default;
        virtual void Apply(IItemEffectTarget& target) const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Item::IItemEffect, 0)
