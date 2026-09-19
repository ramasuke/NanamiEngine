#pragma once
#include <array>
#include <string_view>

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarType final : int
    {
        SwordMan = 0,
        Gunner = 1,
        MagicCaster = 2,
    };

    constexpr std::array PLAYER_AVATAR_TYPES
    {
        PlayerAvatarType::SwordMan,
        PlayerAvatarType::Gunner,
        PlayerAvatarType::MagicCaster,
    };

    constexpr std::string_view ToString(const PlayerAvatarType type)
    {
        switch (type)
        {
        case PlayerAvatarType::SwordMan   : return "SwordMan";
        case PlayerAvatarType::Gunner     : return "Gunner";
        case PlayerAvatarType::MagicCaster: return "MagicCaster";
        }

        return "Unknown";
    }
}
