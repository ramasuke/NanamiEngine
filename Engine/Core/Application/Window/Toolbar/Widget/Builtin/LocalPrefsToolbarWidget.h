#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief LocalPrefs を subPath ごとにまとめて編集するメニュー */
    class NANAMI_API LocalPrefsToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
