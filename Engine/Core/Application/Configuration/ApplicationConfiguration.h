#pragma once
#include "../../Network/Mode/NetworkSystem_Mode.h"

namespace NanamiEngine::Core::Application::Configuration
{
    enum class ApplicationMode
    {
        Editor,
        Game
    };
    
    constexpr auto APPLICATION_MODE  = ApplicationMode::Editor;
    constexpr auto NETWORK_MODE      = Network::Mode::Server;
    
    constexpr auto WINDOW_WIDTH_SIZE  = 1920;
    constexpr auto WINDOW_HEIGHT_SIZE = 1080;
    constexpr auto WINDOW_COLOR_SCALE = 16;
}
