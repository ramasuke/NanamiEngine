#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarInputPhase : uint8_t
    {
        Pressed,
        Holding,
        NotHolding,
    };
}
