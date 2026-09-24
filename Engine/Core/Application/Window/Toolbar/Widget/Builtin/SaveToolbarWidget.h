#pragma once
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief シーンとアセットの保存。再生中は出さない */
    class SaveToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        [[nodiscard]] bool IsVisible() const override;
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
