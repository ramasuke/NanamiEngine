#pragma once

namespace NanamiEngine::Core::PopupWindow
{
    class PopupWindowGroup;
}

namespace NanamiEngine::Core
{
    class EditorToolbarWindow final
    {
    public:
        static void OnDraw(PopupWindow::PopupWindowGroup& popupWindows);
    };
}
