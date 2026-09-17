#pragma once
#include <string>

#include "AutoMcpJson.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    struct AutoMcpCapture
    {
        int sourceHandle = -1;
        int width        = 0;
        int height       = 0;
    };

    class AutoMcpServer;

    class AutoMcpScreenshot final
    {
        friend class AutoMcpServer;

        AutoMcpScreenshot() = delete;

        /** @brief 現在の描画先をコピーする。描画先は切り替えないので ImGui 描画前でも呼べる */
        [[nodiscard]] static AutoMcpCapture Grab();
        /** @brief 縮小して保存し、保存先を result に書く。描画先を一時的に切り替えるので ImGui 描画後に呼ぶ */
        static void Save(const AutoMcpCapture& capture, const std::string& format, int maxWidth, int quality, JsonValue& result, JsonAllocator& allocator);
        static void Release(AutoMcpCapture& capture);
    };
}
