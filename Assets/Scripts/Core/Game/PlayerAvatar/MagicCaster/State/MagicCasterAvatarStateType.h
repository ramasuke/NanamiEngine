#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar::MagicCaster
{
    enum class MagicCasterAvatarStateType : uint8_t
    {
        Disable = 0,
        Idle = 1,
        Walk = 2,
        Run = 3,
        Jump = 4,
        Floating = 5,
        Cast = 6,
        Hurt = 7,
        Death = 8,
    };
}
