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
    };

    /// State を遷移させない操作
    enum class MagicCasterAvatarStateAction : uint8_t
    {
        Move,
        LockOn,
        LockOnRelease,
    };

    class IMagicCasterAvatarTransitionVisitor
        : public IPlayerAvatarTransitionVisitor<MagicCasterAvatarStateType, MagicCasterAvatarInput, MagicCasterAvatarStateAction>
    {
    public:
        /**
         * @brief 基本魔法か持ち込み枠の詠唱へ移る
         * @param isBasicSpellUsable 操作ガイドの表示専用。撃てるかどうかは枠ごとに TryBeginCast が判定する
         */
        virtual bool Cast(bool isBasicSpellUsable) = 0;
    };
}
