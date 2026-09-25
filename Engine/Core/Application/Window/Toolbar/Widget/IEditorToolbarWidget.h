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

    /** @brief エディタのツールバーに並ぶ 1 要素。REGISTER_EDITOR_TOOLBAR_WIDGET で登録する */
    class NANAMI_API IEditorToolbarWidget
    {
    public:
        virtual ~IEditorToolbarWidget() = default;

        /** @brief false の間は OnDraw を呼ばず、場所も詰める */
        [[nodiscard]] virtual bool IsVisible() const { return true; }
        virtual void OnDraw(EditorToolbarWidgetContext& context) = 0;
    };
}
