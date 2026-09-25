#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief シーンとアセットの保存。再生中は出さない */
    class NANAMI_API SaveToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        [[nodiscard]] bool IsVisible() const override;
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
