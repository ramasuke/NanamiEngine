#pragma once

namespace NanamiEngine::Core::Application::Configuration
{
    enum class ApplicationMode
    {
        Editor,
        Game
    };

#if defined(NANAMI_GAME_BUILD)
    constexpr auto APPLICATION_MODE = ApplicationMode::Game;
#else
    constexpr auto APPLICATION_MODE = ApplicationMode::Editor;
#endif
}
