#include "PopupWindowMenuToolbarWidget.h"

#include <vector>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../Popup/Factory/PopupWindowFactory.h"
#include "../../../Popup/Group/PopupWindowGroup.h"
#include "../../../../../../Module/Gui/StaticReflection/Engine_Module_StaticReflection.h"

namespace NanamiEngine::Core::Toolbar
{
    void PopupWindowMenuToolbarWidget::OnDraw(EditorToolbarWidgetContext& context)
    {
        if (ImGui::Button("PopupWindow"))
        {
            ImGui::OpenPopup("WindowPopup");
        }

        if (ImGui::BeginPopup("WindowPopup"))
        {
            const auto& registry = PopupWindow::PopupWindowFactory::Instance();
            std::vector<Module::StaticReflection::CategoryMenuItem> menuItems;
            for (const auto& [name, factory] : registry.GetAll())
            {
                menuItems.push_back({ registry.GetCategories().at(name), name, true, [&popupWindows = context.popupWindows, &createWindow = factory]
                {
                    popupWindows.InjectWindow(createWindow());
                } });
            }
            Module::StaticReflection::DrawCategoryMenu(menuItems);
            ImGui::EndPopup();
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(PopupWindowMenuToolbarWidget, 400)
}
