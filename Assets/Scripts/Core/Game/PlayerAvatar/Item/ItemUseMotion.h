#pragma once
#include <array>
#include <string_view>

namespace GameCore::PlayerAvatar::Item
{
    // アイテムを使うときのモーション。Instant はモーション無しでその場で効く
    enum class ItemUseMotion : int
    {
        Instant = 0,
        Drink   = 1,
        Eat     = 2,
        Place   = 3,
    };

    constexpr std::array ITEM_USE_MOTIONS
    {
        ItemUseMotion::Instant,
        ItemUseMotion::Drink,
        ItemUseMotion::Eat,
        ItemUseMotion::Place,
    };

    constexpr std::string_view ToString(const ItemUseMotion motion)
    {
        switch (motion)
        {
        case ItemUseMotion::Instant: return "Instant";
        case ItemUseMotion::Drink:   return "Drink";
        case ItemUseMotion::Eat:     return "Eat";
        case ItemUseMotion::Place:   return "Place";
        }
        return "Unknown";
    }
}
