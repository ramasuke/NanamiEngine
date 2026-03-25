#pragma once
#include "ImGuiHelper.h"

namespace NanamiEngine::Module::Gui::Graph
{
    struct NodeVisualStyle final
    {
        explicit NodeVisualStyle(
            ImU32 background,
            ImU32 border,
            ImU32 port,
            ImU32 text);

        [[nodiscard]] ImU32       BackgroundColor() const noexcept { return backgroundColor_; }
        [[nodiscard]] ImU32       BorderColor    () const noexcept { return borderColor_;     }
        [[nodiscard]] ImU32       PortColor      () const noexcept { return portColor_;       }
        [[nodiscard]] ImU32       TextColor      () const noexcept { return textColor_;       }
        
    private:
        ImU32 backgroundColor_;
        ImU32 borderColor_;
        ImU32 portColor_;
        ImU32 textColor_;
    };
}
