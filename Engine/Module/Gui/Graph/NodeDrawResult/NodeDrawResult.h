#pragma once

namespace NanamiEngine::Module::Gui::Graph
{
    struct NodeDrawResult final
    {
        bool isOnBeginDragOutput_      = false;
        bool isInputHoveredReleased_ = false;
    };
}
