#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include "AutoMcpJson.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpServer;

    /** @brief コマンドを実行するフレーム内のタイミング */
    enum class AutoMcpPhase
    {
        /** ImGui::NewFrame 直後。ImGui ウィンドウの位置・サイズ変更など */
        FrameBegin,
        /** ImGui 描画後。受信したフレームでそのまま実行する */
        FrameEnd,
    };

    struct AutoMcpCommand
    {
        AutoMcpPhase phase;
        std::function<void(const JsonValue& args, JsonValue& result, JsonAllocator& allocator)> handler;
    };

    /** @brief "screenshot" 以外の全コマンドの表。screenshot は描画タイミングに依存するので AutoMcpServer が直接扱う */
    class AutoMcpCommandTable final
    {
        friend class AutoMcpServer;

        AutoMcpCommandTable() = delete;

        [[nodiscard]] static const std::unordered_map<std::string, AutoMcpCommand>& Get();
    };
}
