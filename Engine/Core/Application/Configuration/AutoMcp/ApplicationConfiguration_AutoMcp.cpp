#include "ApplicationConfiguration_AutoMcp.h"

#include <algorithm>

#include "../ApplicationConfiguration.h"
#include "../../AutoMcp/AutoMcpServer.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto AUTO_MCP_DEFAULT_ENABLED              = false;
    constexpr auto AUTO_MCP_DEFAULT_PORT                 = 47321;
    constexpr auto AUTO_MCP_DEFAULT_SCREENSHOT_MAX_WIDTH = 1280;
    constexpr auto AUTO_MCP_MIN_PORT                     = 1024;
    constexpr auto AUTO_MCP_MAX_PORT                     = 65535;
    constexpr auto AUTO_MCP_MIN_SCREENSHOT_WIDTH         = 160;
    constexpr auto AUTO_MCP_MAX_SCREENSHOT_WIDTH         = 7680;

    bool AutoMcpConfiguration::enabled_            = AUTO_MCP_DEFAULT_ENABLED;
    int  AutoMcpConfiguration::port_               = AUTO_MCP_DEFAULT_PORT;
    int  AutoMcpConfiguration::screenshotMaxWidth_ = AUTO_MCP_DEFAULT_SCREENSHOT_MAX_WIDTH;

    constexpr auto AUTO_MCP_CONFIG_PATH              = "AutoMcp/";
    constexpr auto AUTO_MCP_ENABLED_KEY              = "Enabled";
    constexpr auto AUTO_MCP_PORT_KEY                 = "Port";
    constexpr auto AUTO_MCP_SCREENSHOT_MAX_WIDTH_KEY = "ScreenshotMaxWidth";

    void AutoMcpConfiguration::Load()
    {
        enabled_            = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(AUTO_MCP_CONFIG_PATH, AUTO_MCP_ENABLED_KEY,              AUTO_MCP_DEFAULT_ENABLED);
        port_               = Module::ProjectConfig::LoadOrDefaultWithPath<int> (AUTO_MCP_CONFIG_PATH, AUTO_MCP_PORT_KEY,                 AUTO_MCP_DEFAULT_PORT);
        screenshotMaxWidth_ = Module::ProjectConfig::LoadOrDefaultWithPath<int> (AUTO_MCP_CONFIG_PATH, AUTO_MCP_SCREENSHOT_MAX_WIDTH_KEY, AUTO_MCP_DEFAULT_SCREENSHOT_MAX_WIDTH);

        port_               = std::clamp(port_,               AUTO_MCP_MIN_PORT,             AUTO_MCP_MAX_PORT);
        screenshotMaxWidth_ = std::clamp(screenshotMaxWidth_, AUTO_MCP_MIN_SCREENSHOT_WIDTH, AUTO_MCP_MAX_SCREENSHOT_WIDTH);
    }

    void AutoMcpConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<bool>(AUTO_MCP_CONFIG_PATH, AUTO_MCP_ENABLED_KEY,              enabled_);
        Module::ProjectConfig::SaveWithPath<int> (AUTO_MCP_CONFIG_PATH, AUTO_MCP_PORT_KEY,                 port_);
        Module::ProjectConfig::SaveWithPath<int> (AUTO_MCP_CONFIG_PATH, AUTO_MCP_SCREENSHOT_MAX_WIDTH_KEY, screenshotMaxWidth_);
    }

    bool AutoMcpConfiguration::IsEnabled()
    {
        if (APPLICATION_MODE != ApplicationMode::Editor)
            return false;

        return enabled_;
    }

    int AutoMcpConfiguration::GetPort()
    {
        return port_;
    }

    int AutoMcpConfiguration::GetScreenshotMaxWidth()
    {
        return screenshotMaxWidth_;
    }

    void AutoMcpConfiguration::DrawConfigGUI()
    {
        auto& server = AutoMcp::AutoMcpServer::Instance();

        ImGui::Text("Claude Code (MCP)");
        ImGui::Separator();

        bool changed = false;
        bool restart = false;

        if (ImGui::Checkbox("Enable AutoMCP", &enabled_))
        {
            changed = true;
            restart = true;
        }

        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Port", &port_, 0, 0);
        // 入力途中の値でリスナーを作り直さないよう、確定時にだけ反映する
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            port_   = std::clamp(port_, AUTO_MCP_MIN_PORT, AUTO_MCP_MAX_PORT);
            changed = true;
            restart = true;
        }

        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Screenshot Max Width", &screenshotMaxWidth_, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            screenshotMaxWidth_ = std::clamp(screenshotMaxWidth_, AUTO_MCP_MIN_SCREENSHOT_WIDTH, AUTO_MCP_MAX_SCREENSHOT_WIDTH);
            changed = true;
        }

        ImGui::Spacing();
        if (server.IsListening())
            ImGui::Text("Listening on 127.0.0.1:%d  (clients: %d)", server.ListeningPort(), server.ClientCount());
        else
            ImGui::TextDisabled("Not listening");

        if (!server.LastError().empty())
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", server.LastError().c_str());

        ImGui::Spacing();
        ImGui::TextDisabled("* Claude Code connects through .mcp.json (python -m tools.automcp serve)");
        ImGui::TextDisabled("* If you change the port, set NANAMI_AUTOMCP_PORT for the bridge too");

        if (changed)
            Save();

        if (restart)
            server.ApplyConfiguration();
    }
}
