#include "WindowDisplayMode.h"

namespace NanamiEngine::Core::Application::Display
{
    const char* ToString(const WindowDisplayMode mode)
    {
        switch (mode)
        {
        case WindowDisplayMode::Windowed:   return "Windowed";
        case WindowDisplayMode::Borderless: return "Borderless";
        case WindowDisplayMode::Fullscreen: return "Fullscreen";
        }
        return "Windowed";
    }

    WindowDisplayMode WindowDisplayModeFromString(const std::string& text, const WindowDisplayMode fallback)
    {
        for (const auto mode : { WindowDisplayMode::Windowed, WindowDisplayMode::Borderless, WindowDisplayMode::Fullscreen })
        {
            if (text == ToString(mode))
                return mode;
        }
        return fallback;
    }
}
