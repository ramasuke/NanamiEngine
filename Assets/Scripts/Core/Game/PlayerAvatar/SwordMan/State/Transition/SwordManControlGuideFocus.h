#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar::SwordMan
{
    /// 操作ガイドのどの行を強調するか。値は操作ガイドの行と一対一で対応する
    enum class SwordManControlGuideFocus : std::uint8_t
    {
        None,
        Move,
        Attack,
        ChargeAttack,
        Run,
        Jump,
        AvoidRolling,
        LockOn,
        Interact,
    };

    struct SwordManControlGuideFocusState
    {
        SwordManControlGuideFocus target = SwordManControlGuideFocus::None;
        /// 課題を達成した直後。強調をチェック表示に切り替える
        bool isCleared = false;
    };
}
