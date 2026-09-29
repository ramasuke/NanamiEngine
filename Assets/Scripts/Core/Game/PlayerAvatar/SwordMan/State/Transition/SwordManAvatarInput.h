#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar::SwordMan
{
    enum class SwordManAvatarInput : uint8_t
    {
        Move,
        Run,
        Jump,
        AvoidRolling,
        NormalAttack,
        DashAttack,
        Chat,
    };
}
