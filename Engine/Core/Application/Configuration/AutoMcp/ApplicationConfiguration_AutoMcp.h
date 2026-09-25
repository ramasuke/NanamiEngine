#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpServer;
}

namespace NanamiEngine::Core::Application::Configuration
{
    /** @brief Claude Code などの MCP ブリッジ (tools/automcp) からエディタを操作するための受付設定 */
    class NANAMI_API AutoMcpConfiguration final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpServer;

    public:
        static void Load();
        static void Save();

        static void DrawConfigGUI();

    private:
        [[nodiscard]] static bool IsEnabled();
        [[nodiscard]] static int  GetPort();
        [[nodiscard]] static int  GetScreenshotMaxWidth();

        static bool enabled_;
        static int  port_;
        static int  screenshotMaxWidth_;
    };
}
