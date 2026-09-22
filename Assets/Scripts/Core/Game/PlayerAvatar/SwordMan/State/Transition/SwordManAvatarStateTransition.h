#pragma once
#include <cstdint>

#include "../SwordManAvatarStateType.h"
#include "../../../State/Transition/PlayerAvatarStateTransition.h"

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

    /// State を遷移させない操作
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
        OpenMenu,
    };

    using ISwordManAvatarTransitionVisitor =
        IPlayerAvatarTransitionVisitor<SwordManAvatarStateType, SwordManAvatarInput, SwordManAvatarStateAction>;
}
