#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../ApplicationBase.h"

namespace NanamiEngine::Core::Application::Game
{
    class NANAMI_API GameApplication final : public ApplicationBase
    {
    public:
        GameApplication();

    private:
        void OnFrame() override;
        void OnExit () override;
    };
}
