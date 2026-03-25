#pragma once
#include "../../../../../Module/GameObject/Interface/IGameObject.h"
#include "../Interface/IPopupWindow.h"
#include "../Factory/PopupWindowFactory.h"

namespace NanamiEngine::Core::PopupWindow
{
    class InspectorWindow final : public IPopupWindow
    {
    public:
        InspectorWindow();
        ::Guid& Guid()  override { return guid_; }
        void OnDraw(PopupWindowDrawGuiContext context)   override;
        void TryAddDisplayObject(const std::weak_ptr<Module::Object::IObject>& gameObject);
        
    private:
        static int counter_;
        int id_;
        ::Guid guid_;
        std::weak_ptr<Module::Object::IObject> displayGameObject_;
        bool isLockedContent_ = false;
    };
    
    REGISTER_POPUP_WINDOW(InspectorWindow);
}
