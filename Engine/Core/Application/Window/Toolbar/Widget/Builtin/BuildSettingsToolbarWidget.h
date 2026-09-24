#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief Build Settings を開くボタンと、ビルド中の進み具合 */
    class BuildSettingsToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
