#include "EditorToolbarWindow.h"

#include <map>

#include "ImGuiHelper.h"
#include "../../../FileSystem/Directory/Directory.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "../../Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../ApplicationBase.h"
#include "../../../../Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../Main/Factory/MainWindowFactory.h"
#include "../Main/Game/GameWindow.h"
#include "../Popup/Group/PopupWindowGroup.h"
#include "../Popup/Factory/PopupWindowFactory.h"

void Core::EditorToolbarWindow::OnDraw(PopupWindow::PopupWindowGroup& popupWindows)
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

    if (ImGui::Button("Config"))
    {
        ImGui::OpenPopup("ConfigWindow");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("ConfigWindow"))
    {
        // === Application ===
        ImGui::Text("Application");
        ImGui::Separator();

        int w = Application::Configuration::AppConfiguration::GetWindowWidth();
        int h = Application::Configuration::AppConfiguration::GetWindowHeight();
        int s = Application::Configuration::AppConfiguration::GetWindowColorScale();

        ImGui::SetNextItemWidth(100);
        const bool wChanged = ImGui::InputInt("Window Width",  &w);
        ImGui::SetNextItemWidth(100);
        const bool hChanged = ImGui::InputInt("Window Height", &h);
        ImGui::SetNextItemWidth(100);
        const bool sChanged = ImGui::InputInt("Color Scale",   &s);

        if (wChanged || hChanged || sChanged)
        {
            Application::Configuration::AppConfiguration::SetWindowWidth(w);
            Application::Configuration::AppConfiguration::SetWindowHeight(h);
            Application::Configuration::AppConfiguration::SetWindowColorScale(s);
            Application::Configuration::AppConfiguration::Save();
        }
        ImGui::TextDisabled("* Restart required to apply");

        ImGui::Spacing();

        // === Network ===
        ImGui::Text("Network Mode");
        ImGui::Separator();

        auto currentMode = Application::Configuration::NetworkConfiguration::GetMode();
        int  modeIndex   = currentMode == Core::Network::Mode::Server ? 0 : 1;

        if (ImGui::RadioButton("Server", &modeIndex, 0) || ImGui::RadioButton("Client", &modeIndex, 1))
        {
            const auto newMode = modeIndex == 0 ? Core::Network::Mode::Server : Core::Network::Mode::Client;
            Application::Configuration::NetworkConfiguration::SetMode(newMode);
            Application::Configuration::NetworkConfiguration::Save();
        }

        if (currentMode == Core::Network::Mode::Server)
        {
            ImGui::Separator();
            ImGui::Text("Server Type");

            auto currentServerType = Application::Configuration::NetworkConfiguration::GetServerType();
            int  serverTypeIndex   = currentServerType == Core::Network::ServerType::Relay ? 0 : 1;

            if (ImGui::RadioButton("Relay", &serverTypeIndex, 0) ||
                ImGui::RadioButton("Authoritative", &serverTypeIndex, 1))
            {
                const auto newType = serverTypeIndex == 0
                    ? Core::Network::ServerType::Relay
                    : Core::Network::ServerType::Authoritative;
                Application::Configuration::NetworkConfiguration::SetServerType(newType);
                Application::Configuration::NetworkConfiguration::Save();
            }
        }

        ImGui::EndPopup();
    }
    
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

    if (ImGui::Button("LocalPrefs"))
    {
        ImGui::OpenPopup("LocalPrefsWindow");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("LocalPrefsWindow"))
    {
        const auto& prefsList = LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().GetPrefsList();

        // subPath をカテゴリキーとしてグループ化 (アルファベット順、空文字は "General")
        std::map<std::string, std::vector<size_t>> categoryMap;
        for (size_t i = 0; i < prefsList.size(); ++i)
            categoryMap[prefsList[i].subPath].push_back(i);

        for (const auto& [subPath, indices] : categoryMap)
        {
            const std::string header = subPath.empty() ? "General" : subPath;
            if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t idx : indices)
                    prefsList[idx].drawEditGui();
            }
        }

        ImGui::EndPopup();
    }
    ImGui::End();
}
