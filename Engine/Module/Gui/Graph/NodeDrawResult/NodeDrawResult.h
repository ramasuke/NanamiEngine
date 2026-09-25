#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Gui::Graph
{
    struct NANAMI_API NodeDrawResult final
    {
        bool isOnBeginDragOutput_      = false;
        bool isInputHoveredReleased_ = false;
    };
}
