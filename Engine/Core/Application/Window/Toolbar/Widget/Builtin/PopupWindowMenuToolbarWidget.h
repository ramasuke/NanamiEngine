#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief 登録済みの PopupWindow を開くメニュー */
    class PopupWindowMenuToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
