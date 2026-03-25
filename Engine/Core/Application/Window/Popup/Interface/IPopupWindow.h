#pragma once
#include "../../../../../Module/Guid/Guid.h"
#include "../DrawGuiContext/PopupWindowDrawGuiContext.h"

namespace NanamiEngine::Core::PopupWindow
{
    class IPopupWindow
    {
    public:
        virtual ~IPopupWindow() = default;
        virtual Guid& Guid() = 0;
        virtual void OnDraw(PopupWindowDrawGuiContext context) = 0;
    };
}
