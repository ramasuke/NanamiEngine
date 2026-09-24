#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief Config ボタンと、各設定のタブ */
    class ConfigToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
