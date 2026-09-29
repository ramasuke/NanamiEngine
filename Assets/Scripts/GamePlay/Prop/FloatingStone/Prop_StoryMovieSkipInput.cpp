#include "Prop_StoryMovieSkipInput.h"

#include "Engine/Core/Platform/Input/Input.h"

namespace GamePlay::Prop::StoryMovie
{
    namespace
    {
        constexpr unsigned char SKIP_TRIGGER_DEAD_ZONE = 30;

        bool IsSkipInputDown()
        {
            if (NanamiEngine::Platform::Input::Keyboard::IsAnyDown())
                return true;

            const auto xInput = NanamiEngine::Platform::Input::Gamepad::Get();
            if (!xInput.connected)
                return false;

            if (xInput.leftTrigger > SKIP_TRIGGER_DEAD_ZONE || xInput.rightTrigger > SKIP_TRIGGER_DEAD_ZONE)
                return true;

            for (const bool button : xInput.buttons)
            {
                if (button)
                    return true;
            }
            return false;
        }
    }

    bool SkipInput::IsSkipped()
    {
        const bool isDown = IsSkipInputDown();
        isArmed_ |= !isDown;
        return isArmed_ && isDown;
    }
}
