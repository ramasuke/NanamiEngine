#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar::SwordMan
{
    enum class SwordManAvatarStateType : uint8_t
    {
        Disable            = 0,
        Idle               = 1,
        Walk               = 2,
        Run                = 3,
        Jump               = 4,
        Floating           = 5,
        NormalAttack       = 6,
        AttackedShocked    = 7,
        DashAttack         = 8,
        ClimbToTop         = 9,
        ArmStretch         = 10,
        Chatting           = 11,
        Hurt               = 14,
        AvoidRolling       = 15,
        Death              = 16,
        UseCanon           = 17,
        InjuredWalk        = 18,
        InjuredRun         = 19,
        Down               = 20,
        WakeUp             = 21,
    };
}
