#include "InspectorWindow.h"

#include "ImGuiHelper.h"
#include "../../../../../Module/GameObject/Interface/IGameObject.h"

int NanamiEngine::Core::PopupWindow::InspectorWindow::counter_ = 0;

NanamiEngine::Core::PopupWindow::InspectorWindow::InspectorWindow()
{
    id_ = counter_++;
}

void NanamiEngine::Core::PopupWindow::InspectorWindow::OnDraw(PopupWindowDrawGuiContext context)
{
    ImGui::Begin(("Inspector##" + std::to_string(id_)).c_str());
    ImGui::Checkbox("isLock", &isLockedContent_);
    if (const auto gameObject = displayGameObject_.lock())
    {
        gameObject->OnDrawGui();
    }
    ImGui::End();
}

void NanamiEngine::Core::PopupWindow::InspectorWindow::TryAddDisplayObject(
    const std::weak_ptr<Module::Object::IObject>& gameObject)
{
    if (!isLockedContent_)
    {
        displayGameObject_ = gameObject;
    }
}
