#include "EditorToolbarWindow.h"

#include "ImGuiHelper.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "Widget/EditorToolbarWidgetRegistry.h"

void NanamiEngine::Core::EditorToolbarWindow::OnDraw(PopupWindow::PopupWindowGroup& popupWindows)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Application::Configuration::AppConfiguration::GetWindowWidth(), 17), ImGuiCond_Always);
    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings
    );
    ImGui::SameLine();

    // NOTE: 各要素は Widget/ 以下で REGISTER_EDITOR_TOOLBAR_WIDGET して並べる
    Toolbar::EditorToolbarWidgetContext context{ popupWindows };
    Toolbar::EditorToolbarWidgetRegistry::Instance().DrawAll(context);
    ImGui::End();
}
