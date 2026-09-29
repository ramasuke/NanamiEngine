#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "EditorToolbarWidgetContext.h"

namespace NanamiEngine::Core::Toolbar
{
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
