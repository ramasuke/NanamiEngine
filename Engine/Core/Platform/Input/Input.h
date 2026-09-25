#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <array>
#include <cstdint>

#include "vec2.hpp"

// キーボード / マウス / ゲームパッドの生入力。DxLib の CheckHitKey / GetMouseInput / GetJoypadXInputState の DxLib を出さない入口
// (docs/HotReload.md §2: ゲームコードは DxLib を直接呼ばない)。列挙の値は DxLib の定数と同じで、.cpp で static_assert している
namespace NanamiEngine::Platform::Input
{
    /** DirectInput のキーコード (DxLib の KEY_INPUT_*) */
    enum class Key : int
    {
        Escape = 0x01,
        Num1 = 0x02, Num2 = 0x03, Num3 = 0x04, Num4 = 0x05, Num5 = 0x06, Num6 = 0x07, Num7 = 0x08, Num8 = 0x09, Num9 = 0x0A, Num0 = 0x0B,
        Minus = 0x0C, Back = 0x0E, Tab = 0x0F,
        Q = 0x10, W = 0x11, E = 0x12, R = 0x13, T = 0x14, Y = 0x15, U = 0x16, I = 0x17, O = 0x18, P = 0x19,
        LBracket = 0x1A, RBracket = 0x1B, Return = 0x1C, LControl = 0x1D,
        A = 0x1E, S = 0x1F, D = 0x20, F = 0x21, G = 0x22, H = 0x23, J = 0x24, K = 0x25, L = 0x26,
        Semicolon = 0x27, LShift = 0x2A,
        Z = 0x2C, X = 0x2D, C = 0x2E, V = 0x2F, B = 0x30, N = 0x31, M = 0x32,
        Comma = 0x33, Period = 0x34, Slash = 0x35, RShift = 0x36, LAlt = 0x38, Space = 0x39,
        F1 = 0x3B, F2 = 0x3C, F3 = 0x3D, F4 = 0x3E, F5 = 0x3F, F6 = 0x40, F7 = 0x41, F8 = 0x42, F9 = 0x43, F10 = 0x44,
        Numpad7 = 0x47, Numpad8 = 0x48, Numpad9 = 0x49, Numpad4 = 0x4B, Numpad5 = 0x4C, Numpad6 = 0x4D,
        Numpad1 = 0x4F, Numpad2 = 0x50, Numpad3 = 0x51, Numpad0 = 0x52,
        F11 = 0x57, F12 = 0x58,
        At = 0x91, NumpadEnter = 0x9C, RControl = 0x9D, RAlt = 0xB8,
        Home = 0xC7, Up = 0xC8, PageUp = 0xC9, Left = 0xCB, Right = 0xCD, End = 0xCF, Down = 0xD0, PageDown = 0xD1,
        Insert = 0xD2, Delete = 0xD3,
    };

    /** マウスボタン (DxLib の MOUSE_INPUT_*、ビットマスク) */
    enum class MouseButton : int
    {
        Left   = 0x0001,
        Right  = 0x0002,
        Middle = 0x0004,
    };

    /** XInput のボタン番号 (DxLib の XINPUT_BUTTON_*、GamepadState::buttons の添字) */
    enum class GamepadButton : int
    {
        DPadUp = 0, DPadDown = 1, DPadLeft = 2, DPadRight = 3,
        Start = 4, Back = 5, LeftThumb = 6, RightThumb = 7,
        LeftShoulder = 8, RightShoulder = 9,
        A = 12, B = 13, X = 14, Y = 15,
    };

    struct NANAMI_API GamepadState
    {
        bool                     connected = false;
        std::array<bool, 16>     buttons{};
        std::uint8_t             leftTrigger  = 0;  // 0..255
        std::uint8_t             rightTrigger = 0;  // 0..255
        std::int16_t             thumbLX = 0, thumbLY = 0, thumbRX = 0, thumbRY = 0; // -32768..32767

        [[nodiscard]] bool IsDown(const GamepadButton button) const { return buttons[static_cast<int>(button)]; }
        /** @brief ボタン・トリガー (deadZone 超) ・スティック (deadZone 超) のどれかが触られているか */
        [[nodiscard]] bool IsAnyDown(std::uint8_t triggerDeadZone = 30, std::int16_t thumbDeadZone = 8000) const;
    };

    namespace Keyboard
    {
        [[nodiscard]] NANAMI_API bool IsDown(Key key);
        /** @brief キーボードのどれかが押されているか (マウス・パッドは見ない) */
        [[nodiscard]] NANAMI_API bool IsAnyDown();
        /** @brief 数字キー (上段 1..0 とテンキー) のどちらかで digit (0..9) が押されているか */
        [[nodiscard]] NANAMI_API bool IsDigitDown(int digit);
    }

    namespace Mouse
    {
        /** @brief 押されているボタンのビットマスク (MouseButton の値の OR) */
        [[nodiscard]] NANAMI_API int        Buttons();
        [[nodiscard]] NANAMI_API bool       IsDown(MouseButton button);
        [[nodiscard]] NANAMI_API glm::ivec2 Position();
        /** @brief ホイールの累積回転量。reset=true で読んだ後に 0 に戻す */
        [[nodiscard]] NANAMI_API int        WheelRotation(bool reset = false);
    }

    namespace Gamepad
    {
        /** @param index 0 = 1 つ目のパッド */
        [[nodiscard]] NANAMI_API GamepadState Get(int index = 0);
    }

    /** @brief キーボード・マウス・パッドのどれかが押されているか (DxLib の CheckHitKeyAll()) */
    [[nodiscard]] NANAMI_API bool IsAnyDeviceDown();
    /** @brief メインウィンドウがアクティブか */
    [[nodiscard]] NANAMI_API bool IsWindowActive();
}
