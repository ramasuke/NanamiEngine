#include "EditorToolbarWindow.h"

#include <map>
#include <vector>

#include "ImGuiHelper.h"
#include "../../../FileSystem/Directory/Directory.h"
#include "../../Build/GameBuilder.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "../../Configuration/AutoMcp/ApplicationConfiguration_AutoMcp.h"
#include "../../Configuration/CodeEditor/ApplicationConfiguration_CodeEditor.h"
#include "../../Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../../Configuration/GameWindow/ApplicationConfiguration_GameWindow.h"
#include "../../Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../ApplicationBase.h"
#include "../../../../Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../Module/Gui/StaticReflection/Engine_Module_StaticReflection.h"
#include "../../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../Main/Factory/MainWindowFactory.h"
#include "../Main/Game/GameWindow.h"
#include "../Popup/BuildSettings/BuildSettingsWindow.h"
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
            if (ImGui::BeginTabItem("Debug Draw"))
            {
                Application::Configuration::DebugDrawConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Game Window"))
            {
                Application::Configuration::GameWindowConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Code Editor"))
            {
                Application::Configuration::CodeEditorConfiguration::DrawConfigGUI();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("AutoMCP"))
            {
                Application::Configuration::AutoMcpConfiguration::DrawConfigGUI();
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

    if (ImGui::Button("Build Settings"))
    {
        popupWindows.Catch<PopupWindow::BuildSettingsWindow>().front()->RequestFocus();
    }
    ImGui::SameLine();

    // Build Settings を閉じていても進み具合が分かるよう、ビルド中はツールバーにも出す
    if (auto& gameBuilder = Application::Build::GameBuilder::Instance(); gameBuilder.IsBusy())
    {
        ImGui::Text("Building: %s %s", gameBuilder.PhaseLabel(), gameBuilder.ElapsedLabel().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Cancel Build"))
        {
            gameBuilder.Cancel();
        }
        ImGui::SameLine();
    }

    if (ImGui::Button("PopupWindow")) {
        ImGui::OpenPopup("WindowPopup");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("WindowPopup"))
    {
        const auto& registry = PopupWindow::PopupWindowFactory::Instance();
        std::vector<NanamiEngine::Module::StaticReflection::CategoryMenuItem> menuItems;
        for (const auto& [name, factory] : registry.GetAll())
        {
            menuItems.push_back({ registry.GetCategories().at(name), name, true, [&popupWindows, &createWindow = factory]
            {
                popupWindows.InjectWindow(createWindow());
            } });
        }
        NanamiEngine::Module::StaticReflection::DrawCategoryMenu(menuItems);
        ImGui::EndPopup();
    }
    
    if (ImGui::Button("MainWindow"))
    {
        ImGui::OpenPopup("MainWindowPopup");
    }
    ImGui::SameLine();

    if (ImGui::BeginPopup("MainWindowPopup"))
    {
        const auto& registry = MainWindow::MainWindowFactory::Instance();
        std::vector<NanamiEngine::Module::StaticReflection::CategoryMenuItem> menuItems;
        for (const auto& [name, loader] : registry.GetLoaders())
        {
            menuItems.push_back({ registry.GetCategories().at(name), name, true, [&loadWindow = loader]
            {
                Application::ApplicationBase::OnChangeWindow(loadWindow());
            } });
        }
        NanamiEngine::Module::StaticReflection::DrawCategoryMenu(menuItems);
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
