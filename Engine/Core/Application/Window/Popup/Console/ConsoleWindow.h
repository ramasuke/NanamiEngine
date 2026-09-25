#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../Interface/IPopupWindow.h"
#include "../Factory/PopupWindowFactory.h"
#include "../../../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::PopupWindow
{
    class NANAMI_API ConsoleWindow final : public IPopupWindow
    {
    public:
        ConsoleWindow();
        ::Guid& Guid() override { return guid_; }
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context) override;

    private:
        //NOTE: ImGUIのラベル情報のために現在開いているConsoleWindowの数をカウントする
        static int counter_;
        int id_;
        ::Guid guid_;
        char searchBuffer_[128] = {};
        bool showInfo_    = true;
        bool showWarning_ = true;
        bool showError_   = true;
        bool collapse_    = false;
        bool autoScroll_  = true;
    };

    REGISTER_POPUP_WINDOW(ConsoleWindow, "General");
}
