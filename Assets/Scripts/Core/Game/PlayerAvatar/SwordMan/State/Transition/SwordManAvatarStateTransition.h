#pragma once
#include <cstdint>

#include "../SwordManAvatarStateType.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    enum class SwordManAvatarControlAcceptance : uint8_t
    {
        None,
        /// 一瞬で終わるので、受け付ける操作は直前の State のものとみなす
        Momentary,
        Accept,
    };

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

    enum class SwordManAvatarInputPhase : uint8_t
    {
        Pressed,
        Holding,
        NotHolding,
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
    };

    /**
     * @brief State が起こしうる遷移と State 内の操作を、評価順に受け取る
     * @note 引数は呼んだ時点で評価されるので、前の遷移で状態が変わった後の値が次の宣言に渡る
     * @return 実際に遷移したか。表示など遷移を行わない実装は常に false を返す
     */
    class ISwordManAvatarTransitionVisitor
    {
    public:
        virtual ~ISwordManAvatarTransitionVisitor() = default;

        virtual bool Automatic(SwordManAvatarStateType to, bool condition) = 0;
        /** @param isUsable 入力以外の遷移条件。操作ガイドの使用可否表示にも使われる */
        virtual bool OnInput(SwordManAvatarStateType to, SwordManAvatarInput input, SwordManAvatarInputPhase phase, bool isUsable) = 0;
        /** @param isReady 使用可否としては見せないタイミング条件 */
        virtual bool OnInputWhenReady(SwordManAvatarStateType to, SwordManAvatarInput input, SwordManAvatarInputPhase phase, bool isUsable, bool isReady) = 0;
        virtual void Action(SwordManAvatarStateAction action, bool isUsable) = 0;
    };
}
