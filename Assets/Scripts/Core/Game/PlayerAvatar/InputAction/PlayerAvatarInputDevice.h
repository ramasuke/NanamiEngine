#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarInputDevice : std::uint8_t
    {
        KeyboardMouse,
        Gamepad,
    };
}
