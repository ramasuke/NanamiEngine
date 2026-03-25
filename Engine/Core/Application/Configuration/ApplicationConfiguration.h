#pragma once

namespace NanamiEngine::Core::Application::Configuration
{
    enum class ApplicationMode
    {
        Editor,
        Game
    };
    
    constexpr auto APPLICATION_MODE  = ApplicationMode::Editor;
    
    constexpr auto WINDOW_WIDTH_SIZE  = 1920;
    constexpr auto WINDOW_HEIGHT_SIZE = 1080;
    constexpr auto WINDOW_COLOR_SCALE = 16;
}
