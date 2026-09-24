#include "MainWindowMenuToolbarWidget.h"

#include <vector>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../ApplicationBase.h"
#include "../../../Main/Factory/MainWindowFactory.h"
#include "../../../../../../Module/Gui/StaticReflection/Engine_Module_StaticReflection.h"

namespace NanamiEngine::Core::Toolbar
{
    void MainWindowMenuToolbarWidget::OnDraw(EditorToolbarWidgetContext&)
    {
        if (ImGui::Button("MainWindow"))
        {
            ImGui::OpenPopup("MainWindowPopup");
        }

        if (ImGui::BeginPopup("MainWindowPopup"))
        {
            const auto& registry = MainWindow::MainWindowFactory::Instance();
            std::vector<Module::StaticReflection::CategoryMenuItem> menuItems;
            for (const auto& [name, loader] : registry.GetLoaders())
            {
                menuItems.push_back({ registry.GetCategories().at(name), name, true, [&loadWindow = loader]
                {
                    Application::ApplicationBase::OnChangeWindow(loadWindow());
                } });
            }
            Module::StaticReflection::DrawCategoryMenu(menuItems);
            ImGui::EndPopup();
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(MainWindowMenuToolbarWidget, 500)
}
