#include "Input.h"

#include <cstdlib>

#include "DxLib.h"

namespace NanamiEngine::Platform::Input
{
    namespace
    {
        constexpr int KeyCode(const Key key) { return static_cast<int>(key); }

        // 列挙の値が DxLib と一致していること (代表値だけ。他は同じ表から写している)
        static_assert(KeyCode(Key::Escape)   == KEY_INPUT_ESCAPE);
        static_assert(KeyCode(Key::Num1)     == KEY_INPUT_1);
        static_assert(KeyCode(Key::Num0)     == KEY_INPUT_0);
        static_assert(KeyCode(Key::Back)     == KEY_INPUT_BACK);
        static_assert(KeyCode(Key::Return)   == KEY_INPUT_RETURN);
        static_assert(KeyCode(Key::LControl) == KEY_INPUT_LCONTROL);
        static_assert(KeyCode(Key::A)        == KEY_INPUT_A);
        static_assert(KeyCode(Key::LShift)   == KEY_INPUT_LSHIFT);
        static_assert(KeyCode(Key::Z)        == KEY_INPUT_Z);
        static_assert(KeyCode(Key::Space)    == KEY_INPUT_SPACE);
        static_assert(KeyCode(Key::Numpad0)  == KEY_INPUT_NUMPAD0);
        static_assert(KeyCode(Key::Numpad7)  == KEY_INPUT_NUMPAD7);
        static_assert(KeyCode(Key::At)       == KEY_INPUT_AT);
        static_assert(KeyCode(Key::Up)       == KEY_INPUT_UP);
        static_assert(KeyCode(Key::Left)     == KEY_INPUT_LEFT);
        static_assert(KeyCode(Key::Right)    == KEY_INPUT_RIGHT);
        static_assert(KeyCode(Key::Down)     == KEY_INPUT_DOWN);
        static_assert(KeyCode(Key::Delete)   == KEY_INPUT_DELETE);
        static_assert(static_cast<int>(MouseButton::Left)   == MOUSE_INPUT_LEFT);
        static_assert(static_cast<int>(MouseButton::Right)  == MOUSE_INPUT_RIGHT);
        static_assert(static_cast<int>(MouseButton::Middle) == MOUSE_INPUT_MIDDLE);
        static_assert(static_cast<int>(GamepadButton::DPadUp)        == XINPUT_BUTTON_DPAD_UP);
        static_assert(static_cast<int>(GamepadButton::DPadRight)     == XINPUT_BUTTON_DPAD_RIGHT);
        static_assert(static_cast<int>(GamepadButton::Start)         == XINPUT_BUTTON_START);
        static_assert(static_cast<int>(GamepadButton::RightThumb)    == XINPUT_BUTTON_RIGHT_THUMB);
        static_assert(static_cast<int>(GamepadButton::LeftShoulder)  == XINPUT_BUTTON_LEFT_SHOULDER);
        static_assert(static_cast<int>(GamepadButton::RightShoulder) == XINPUT_BUTTON_RIGHT_SHOULDER);
        static_assert(static_cast<int>(GamepadButton::A) == XINPUT_BUTTON_A);
        static_assert(static_cast<int>(GamepadButton::B) == XINPUT_BUTTON_B);
        static_assert(static_cast<int>(GamepadButton::X) == XINPUT_BUTTON_X);
        static_assert(static_cast<int>(GamepadButton::Y) == XINPUT_BUTTON_Y);

        constexpr Key TOP_ROW_DIGITS[10] = { Key::Num0, Key::Num1, Key::Num2, Key::Num3, Key::Num4, Key::Num5, Key::Num6, Key::Num7, Key::Num8, Key::Num9 };
        constexpr Key NUMPAD_DIGITS [10] = { Key::Numpad0, Key::Numpad1, Key::Numpad2, Key::Numpad3, Key::Numpad4, Key::Numpad5, Key::Numpad6, Key::Numpad7, Key::Numpad8, Key::Numpad9 };
    }

    bool GamepadState::IsAnyDown(const std::uint8_t triggerDeadZone, const std::int16_t thumbDeadZone) const
    {
        if (!connected)
            return false;
        for (const bool button : buttons)
        {
            if (button)
                return true;
        }
        if (leftTrigger > triggerDeadZone || rightTrigger > triggerDeadZone)
            return true;
        for (const std::int16_t thumb : { thumbLX, thumbLY, thumbRX, thumbRY })
        {
            if (thumb > thumbDeadZone || thumb < -thumbDeadZone)
                return true;
        }
        return false;
    }

    bool Keyboard::IsDown(const Key key)
    {
        return CheckHitKey(KeyCode(key)) != 0;
    }

    bool Keyboard::IsAnyDown()
    {
        return CheckHitKeyAll(DX_CHECKINPUT_KEY) != 0;
    }

    bool Keyboard::IsDigitDown(const int digit)
    {
        if (digit < 0 || digit > 9)
            return false;
        return IsDown(TOP_ROW_DIGITS[digit]) || IsDown(NUMPAD_DIGITS[digit]);
    }

    int Mouse::Buttons()
    {
        return GetMouseInput();
    }

    bool Mouse::IsDown(const MouseButton button)
    {
        return (GetMouseInput() & static_cast<int>(button)) != 0;
    }

    glm::ivec2 Mouse::Position()
    {
        int x = 0, y = 0;
        GetMousePoint(&x, &y);
        return { x, y };
    }

    int Mouse::WheelRotation(const bool reset)
    {
        return GetMouseWheelRotVol(reset ? TRUE : FALSE);
    }

    GamepadState Gamepad::Get(const int index)
    {
        GamepadState state;
        XINPUT_STATE raw{};
        // DX_INPUT_PAD1 = 1、以降は連番
        if (GetJoypadXInputState(DX_INPUT_PAD1 + index, &raw) != 0)
            return state;

        state.connected = true;
        for (int i = 0; i < 16; ++i)
            state.buttons[i] = raw.Buttons[i] != 0;
        state.leftTrigger  = raw.LeftTrigger;
        state.rightTrigger = raw.RightTrigger;
        state.thumbLX = raw.ThumbLX;
        state.thumbLY = raw.ThumbLY;
        state.thumbRX = raw.ThumbRX;
        state.thumbRY = raw.ThumbRY;
        return state;
    }

    bool IsAnyDeviceDown()
    {
        return CheckHitKeyAll() != 0;
    }

    bool IsWindowActive()
    {
        return GetWindowActiveFlag() != FALSE;
    }
}
