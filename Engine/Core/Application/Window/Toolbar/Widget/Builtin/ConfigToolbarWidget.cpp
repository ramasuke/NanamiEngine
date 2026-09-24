#include "ConfigToolbarWidget.h"

#include <string>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../ApplicationBase.h"
#include "../../../../Configuration/ApplicationConfiguration.h"
#include "../../../../Configuration/AutoMcp/ApplicationConfiguration_AutoMcp.h"
#include "../../../../Configuration/CodeEditor/ApplicationConfiguration_CodeEditor.h"
#include "../../../../Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../../../../Configuration/GameWindow/ApplicationConfiguration_GameWindow.h"
#include "../../../../Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../../../Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Toolbar
{
    void ConfigToolbarWidget::OnDraw(EditorToolbarWidgetContext&)
    {
        if (ImGui::Button("Config"))
        {
            ImGui::OpenPopup("ConfigWindow");
        }

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
                        catch (const Module::Exception::NanamiException& exception)
                        {
                            Module::LogError("EditorToolbar: アセットの再読み込みに失敗しました: " + std::string(exception.what()));
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
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(ConfigToolbarWidget, 100)
}
