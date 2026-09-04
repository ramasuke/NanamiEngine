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

        [[nodiscard]] std::weak_ptr<Module::Object::IObject> DisplayObject()   const { return displayGameObject_; }
        [[nodiscard]] int                                    SelectionOrder() const { return selectionOrder_; }

    private:
        static int counter_;
        static int selectionCounter_;
        int id_;
        int selectionOrder_ = 0;
        ::Guid guid_;
        std::weak_ptr<Module::Object::IObject> displayGameObject_;
        bool isLockedContent_ = false;
    };
    
    REGISTER_POPUP_WINDOW(InspectorWindow);
}
