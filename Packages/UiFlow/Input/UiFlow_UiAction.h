#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <vector>

#include "Engine/Core/Platform/Input/Input.h"

namespace NanamiEngine::UiFlow
{
    /** @brief メニュー操作の論理アクション */
    enum class UiAction : std::uint8_t
    {
        Up,
        Down,
        Left,
        Right,
        Submit,
        Cancel,
        TabPrev,
        TabNext,
        /** メニューを開く / 閉じる */
        Menu,
        /** 1 文字消す */
        Erase,
        /** 値を上げる / 下げる。上下の移動と別に使う 2 組目の上下 */
        ValueUp,
        ValueDown,
        Count,
    };

    /** @brief スティックを方向キーとして読む向き */
    enum class StickDirection : std::uint8_t
    {
        Up,
        Down,
        Left,
        Right,
        RightStickUp,
        RightStickDown,
        RightStickLeft,
        RightStickRight,
    };

    /** @brief 1 つのアクションに割り当てる入力。どれかが押されていれば押されている */
    struct NANAMI_API UiBinding
    {
        std::vector<Platform::Input::Key>           keys;
        std::vector<Platform::Input::GamepadButton> buttons;
        std::vector<StickDirection>                 sticks;
    };
}
