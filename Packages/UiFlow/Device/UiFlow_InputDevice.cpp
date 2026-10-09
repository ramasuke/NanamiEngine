#include "UiFlow_InputDevice.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Platform/Input/Input.h"

namespace NanamiEngine::UiFlow
{
    namespace
    {
        constexpr std::int16_t INPUT_DEVICE_THUMB_DEAD_ZONE   = 8000;
        constexpr std::uint8_t INPUT_DEVICE_TRIGGER_DEAD_ZONE = 30;

        struct InputDeviceState
        {
            InputDeviceKind current = InputDeviceKind::KeyboardMouse;
            glm::ivec2      previousMouse{ 0, 0 };
            int             previousWheel = 0;
            std::uint64_t   lastFrame     = 0;
            bool            hasUpdated    = false;
        };

        InputDeviceState& DeviceState()
        {
            static InputDeviceState state;
            return state;
        }

        void UpdateInputDevice(InputDeviceState& state)
        {
            const bool isGamepadTouched = Platform::Input::Gamepad::Get()
                .IsAnyDown(INPUT_DEVICE_TRIGGER_DEAD_ZONE, INPUT_DEVICE_THUMB_DEAD_ZONE);

            const glm::ivec2 mouse = Platform::Input::Mouse::Position();
            // NOTE: 累積値をリセットせずに読み、前回との差を取る。リセットするとほかの読み手の分を取ってしまう
            const int wheel = Platform::Input::Mouse::WheelRotation(false);
            const bool isMouseMoved   = state.hasUpdated && mouse != state.previousMouse;
            const bool isWheelRotated = state.hasUpdated && wheel != state.previousWheel;
            state.previousMouse = mouse;
            state.previousWheel = wheel;

            // NOTE: IsAnyDeviceDown はパッドのボタンも含む
            const bool isKeyboardTouched = Platform::Input::IsAnyDeviceDown()
                || Platform::Input::Mouse::Buttons() != 0 || isMouseMoved || isWheelRotated;

            if (isGamepadTouched && !isKeyboardTouched)
                state.current = InputDeviceKind::Gamepad;
            else if (isKeyboardTouched && !isGamepadTouched)
                state.current = InputDeviceKind::KeyboardMouse;
        }
    }

    InputDeviceKind InputDevice::Current()
    {
        auto& state = DeviceState();
        const std::uint64_t frame = Time::FrameCount();
        if (!state.hasUpdated || frame != state.lastFrame)
        {
            UpdateInputDevice(state);
            state.lastFrame  = frame;
            state.hasUpdated = true;
        }
        return state.current;
    }
}
