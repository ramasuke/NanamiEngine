#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::PopupWindow
{
    class PopupWindowGroup;
}

namespace NanamiEngine::Core::Toolbar
{
    struct NANAMI_API EditorToolbarWidgetContext
    {
        PopupWindow::PopupWindowGroup& popupWindows;
    };
}
