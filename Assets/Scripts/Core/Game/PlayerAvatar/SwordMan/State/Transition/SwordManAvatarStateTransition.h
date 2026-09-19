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
        CannonAttack,
        Chat,
        LockOn,
        CycleItemNext,
        CycleItemPrev,
        UseItem,
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

    class ISwordManAvatarTransitionVisitor
        : public IPlayerAvatarTransitionVisitor<SwordManAvatarStateType, SwordManAvatarInput, SwordManAvatarStateAction>
    {
    public:
        /** @param isReady 使用可否としては見せないタイミング条件 */
        virtual bool OnInputWhenReady(SwordManAvatarStateType to, SwordManAvatarInput input, PlayerAvatarInputPhase phase, bool isUsable, bool isReady) = 0;
    };
}
