#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief Build Settings を開くボタンと、ビルド中の進み具合 */
    class NANAMI_API BuildSettingsToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
