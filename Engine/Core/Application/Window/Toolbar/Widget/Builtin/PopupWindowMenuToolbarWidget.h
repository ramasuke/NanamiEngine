#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief 登録済みの PopupWindow を開くメニュー */
    class NANAMI_API PopupWindowMenuToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        void OnDraw(EditorToolbarWidgetContext& context) override;
    };
}
