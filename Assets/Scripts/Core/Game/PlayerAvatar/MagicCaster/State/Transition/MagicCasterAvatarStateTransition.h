#pragma once
#include <cstdint>

#include "../MagicCasterAvatarStateType.h"
#include "../../../State/Transition/PlayerAvatarStateTransition.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    enum class MagicCasterAvatarInput : uint8_t
    {
        Move,
        Run,
        Jump,
        Chat,
        AvoidRolling,
    };

    /// State を遷移させない操作
    enum class MagicCasterAvatarStateAction : uint8_t
    {
        Move,
        LockOn,
        LockOnRelease,
        CycleItem,
        UseItem,
    };

    class IMagicCasterAvatarTransitionVisitor
        : public IPlayerAvatarTransitionVisitor<MagicCasterAvatarStateType, MagicCasterAvatarInput, MagicCasterAvatarStateAction>
    {
    public:
        /**
         * @brief 基本魔法か持ち込み枠の詠唱へ移る
         * @param isBasicSpellUsable 操作ガイドの表示専用。撃てるかどうかは枠ごとに TryBeginCast が判定する
         */
        virtual void Cast(bool isBasicSpellUsable) {}
    };
}
