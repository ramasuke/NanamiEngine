#include "NodeOption.h"

#include <utility>

namespace NanamiEngine::Module::Gui::Graph
{
    NodeOption::NodeOption(const NodeVisualStyle style,
                           std::string name,
                           const bool hasInput,
                           const bool hasOutput,
                           const ImVec2 size)
        : style_(style)
        , name_(std::move(name))
        , hasInput_(hasInput)
        , hasOutput_(hasOutput)
        , size_(size)
    {
    }
}
