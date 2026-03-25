#pragma once
#include <string>

#include "../ImGui/ImGuiHelper.h"
#include "VisualStyle/NodeVisualStyle.h"

namespace NanamiEngine::Module::Gui::Graph
{
    struct NodeOption final
    {
        explicit NodeOption(NodeVisualStyle style,
                            std::string name,
                            bool hasInput,
                            bool hasOutput,
                            ImVec2 size);

        [[nodiscard]] const std::string&     Name     () const noexcept { return name_;      }
        [[nodiscard]] const NodeVisualStyle& Style    () const noexcept { return style_;     }
        [[nodiscard]] bool                   HasInput () const noexcept { return hasInput_;  }
        [[nodiscard]] bool                   HasOutput() const noexcept { return hasOutput_; }
        [[nodiscard]] ImVec2                 Size     () const noexcept { return size_;      }
        
    private:
        NodeVisualStyle style_;
        std::string name_;
        bool hasInput_;
        bool hasOutput_;
        ImVec2 size_;
    };
}
