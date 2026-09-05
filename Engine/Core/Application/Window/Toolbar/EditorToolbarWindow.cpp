#include "EditorToolbarWindow.h"

#include <map>

#include "ImGuiHelper.h"
#include "../../../FileSystem/Directory/Directory.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "../../Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../ApplicationBase.h"
#include "../../../../Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../Module/Log/NanamiEngine_Module_Log.h"
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
        if (ImGui::BeginTabBar("ConfigTabs"))
        {
            if (ImGui::BeginTabItem("Application"))
            {
                if (ImGui::Button("Reload Assets"))
                {
                    try
                    {
                        Application::ApplicationBase::ResetAssetsDirectory();
                    }
                    catch (const NanamiEngine::Module::Exception::NanamiException& exception)
                    {
                        NanamiEngine::Module::LogError("EditorToolbar: アセットの再読み込みに失敗しました: " + std::string(exception.what()));
                    }
                }
                Application::Configuration::AppConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Network"))
            {
                Application::Configuration::NetworkConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Physics"))
            {
                Application::Configuration::PhysicsConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndPopup();
    }
    
    if (!Application::ApplicationBase::GameWindow()->IsPlaying())
    {
        if (ImGui::Button("Save"))
        {
            try
            {
                Application::ApplicationBase::MainWindows    ().OnSave();
                Application::ApplicationBase::AssetsDirectory().OnSave();
            }
            catch (const NanamiEngine::Module::Exception::NanamiException& exception)
            {
                NanamiEngine::Module::LogError("EditorToolbar: 保存に失敗しました: " + std::string(exception.what()));
            }
        }
        ImGui::SameLine();
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
