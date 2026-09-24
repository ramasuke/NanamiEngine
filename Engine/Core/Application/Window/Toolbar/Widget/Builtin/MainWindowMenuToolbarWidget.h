#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief 登録済みの MainWindow に切り替えるメニュー */
    class MainWindowMenuToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
