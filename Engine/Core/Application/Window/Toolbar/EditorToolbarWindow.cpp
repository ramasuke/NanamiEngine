#include "EditorToolbarWindow.h"

#include "ImGuiHelper.h"
#include "../../../FileSystem/Directory/Directory.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "../../ApplicationBase.h"
#include "../Main/Factory/MainWindowFactory.h"
#include "../Main/Game/GameWindow.h"
#include "../Popup/Group/PopupWindowGroup.h"
#include "../Popup/Factory/PopupWindowFactory.h"

void Core::EditorToolbarWindow::OnDraw(PopupWindow::PopupWindowGroup& popupWindows)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Application::Configuration::WINDOW_WIDTH_SIZE, 17), ImGuiCond_Always);
    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings
    );
    ImGui::SameLine();

    if (!Application::ApplicationBase::GameWindow()->IsPlaying())
    {
        if (ImGui::Button("Save"))
        {
            Application::ApplicationBase::MainWindows    ().OnSave();
            Application::ApplicationBase::AssetsDirectory().OnSave();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("PopupWindow")) {
        ImGui::OpenPopup("WindowPopup");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("WindowPopup"))
    {   
        for (const auto& registry = PopupWindow::PopupWindowFactory::Instance(); const auto& [name, popupWindow] : registry.GetAll())
        {
            if (ImGui::Button(name.c_str()))
            {
                popupWindows.InjectWindow(std::move(popupWindow()));
            }
        }
        ImGui::EndPopup();
    }
    
    if (ImGui::Button("MainWindow"))
    {
        ImGui::OpenPopup("MainWindowPopup");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("MainWindowPopup"))
    {
        for (const auto& registry = MainWindow::MainWindowFactory::Instance(); const auto& [name, loadWindow] : registry.GetLoaders())
        {
            if (ImGui::Button(name.c_str()))
            {
                Application::ApplicationBase::OnChangeWindow(loadWindow());
            }
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
