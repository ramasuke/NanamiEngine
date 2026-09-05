#pragma once
#include "../../../../../Module/Guid/Guid.h"
#include "../DrawGuiContext/PopupWindowDrawGuiContext.h"

namespace NanamiEngine::Core::PopupWindow
{
    enum class PopupWindowState
    {
        Open,
        Closed,
    };

    class IPopupWindow
    {
    public:
        virtual ~IPopupWindow() = default;
        virtual Guid& Guid() = 0;
        virtual PopupWindowState OnDraw(PopupWindowDrawGuiContext context) = 0;
    };
}
