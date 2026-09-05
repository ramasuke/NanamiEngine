#pragma once
#include "../Interface/IPopupWindow.h"
#include "../Factory/PopupWindowFactory.h"
#include "../../../../../Module/Network/Engine_Network_PacketLog.h"

namespace NanamiEngine::Core::PopupWindow
{
    class NetworkLoggerWindow final : public IPopupWindow
    {
    public:
        NetworkLoggerWindow();
        ::Guid& Guid() override { return guid_; }
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context) override;

    private:
        //NOTE: ImGUIのラベル情報のために現在開いているNetworkLoggerWindowの数をカウントする
        static int counter_;
        int id_;
        ::Guid guid_;
        char searchBuffer_[128] = {};
        bool showSend_    = true;
        bool showReceive_ = true;
        bool autoScroll_  = true;
    };

    REGISTER_POPUP_WINDOW(NetworkLoggerWindow);
}
