#include "NodeVisualStyle.h"

namespace NanamiEngine::Module::Gui::Graph
{
    NodeVisualStyle::NodeVisualStyle(const ImU32 background,
                                     const ImU32 border,
                                     const ImU32 port,
                                     const ImU32 text)
        : backgroundColor_(background)
        , borderColor_(border)
        , portColor_(port)
        , textColor_(text)
    {
    }
}
