#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar::SwordMan
{
    // NOTE: State を遷移させない操作
    enum class SwordManAvatarStateAction : uint8_t
    {
        Move,
        ComboAttack,
        LockOn,
        LockOnRelease,
        CannonTurn,
        CannonFire,
        CycleItem,
        UseItem,
    };
}
