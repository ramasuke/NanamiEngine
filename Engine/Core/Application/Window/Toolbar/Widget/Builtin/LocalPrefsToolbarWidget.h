#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief LocalPrefs を subPath ごとにまとめて編集するメニュー */
    class LocalPrefsToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
