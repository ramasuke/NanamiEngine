#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../../Module/Guid/Guid.h"
#include "../DrawGuiContext/PopupWindowDrawGuiContext.h"
#include "PopupWindowState.h"

namespace NanamiEngine::Core::PopupWindow
{
    class NANAMI_API IPopupWindow
    {
    public:
        virtual ~IPopupWindow() = default;
        virtual Guid& Guid() = 0;
        virtual PopupWindowState OnDraw(PopupWindowDrawGuiContext context) = 0;
    };
}
