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
        Count,
    };

    /** @brief 左スティックを方向キーとして読む向き */
    enum class StickDirection : std::uint8_t
    {
        None,
        Up,
        Down,
        Left,
        Right,
    };

    /** @brief 1 つのアクションに割り当てる入力。どれかが押されていれば押されている */
    struct NANAMI_API UiBinding
    {
        std::vector<Platform::Input::Key>           keys;
        std::vector<Platform::Input::GamepadButton> buttons;
        StickDirection                              stick = StickDirection::None;
    };
}
