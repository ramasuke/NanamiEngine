#pragma once
#include "../ApplicationBase.h"

namespace NanamiEngine::Core::Application::Game
{
    class GameApplication final : public ApplicationBase
    {
    public:
        GameApplication();

    private:
        void OnFrame() override;
        void OnExit () override;
    };
}
