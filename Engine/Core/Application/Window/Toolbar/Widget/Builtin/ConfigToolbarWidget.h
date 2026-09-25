#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief Config ボタンと、各設定のタブ */
    class NANAMI_API ConfigToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
