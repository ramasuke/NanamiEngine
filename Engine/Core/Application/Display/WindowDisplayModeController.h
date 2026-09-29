#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>

#include "WindowDisplayMode.h"

namespace NanamiEngine::Core::Application::Display
{
    class NANAMI_API WindowDisplayModeController final
    {
    public:
        static void ApplyBeforeInit();
        static void ApplyAfterInit();

        static void Request(WindowDisplayMode mode);
        static void RequestToggle();

        [[nodiscard]] static WindowDisplayMode Current();

        static void OnFrameEnd();

    private:
        static void Apply(WindowDisplayMode mode);
        static void Save();

        static WindowDisplayMode                current_;
        static WindowDisplayMode                lastFullscreen_;
        static std::optional<WindowDisplayMode> pending_;
        static bool                             toggleKeyHeld_;
    };
}
