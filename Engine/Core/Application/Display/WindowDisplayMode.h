#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::Core::Application::Display
{
    /** メインウィンドウの表示方法。描画解像度 (AppConfiguration の WindowWidth/Height) はどのモードでも変わらない */
    enum class WindowDisplayMode
    {
        Windowed,
        Borderless,
        Fullscreen,
    };

    [[nodiscard]] NANAMI_API const char*       ToString(WindowDisplayMode mode);
    [[nodiscard]] NANAMI_API WindowDisplayMode WindowDisplayModeFromString(const std::string& text, WindowDisplayMode fallback);
}
