#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::PopupWindow
{
    class PopupWindowGroup;
}

namespace NanamiEngine::Core
{
    class NANAMI_API EditorToolbarWindow final
    {
    public:
        static void OnDraw(PopupWindow::PopupWindowGroup& popupWindows);
    };
}
