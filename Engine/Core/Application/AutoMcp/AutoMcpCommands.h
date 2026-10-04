#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <functional>
#include <string>
#include <unordered_map>

#include "AutoMcpJson.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpServer;

    /** @brief コマンドを実行するフレームタイミング */
    enum class AutoMcpPhase
    {
        FrameBegin,
        FrameEnd,
    };

    struct NANAMI_API AutoMcpCommand
    {
        AutoMcpPhase phase;
        std::function<void(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)> handler;
    };

    /** @brief screenshot以外の全コマンドの表 */
    class NANAMI_API AutoMcpCommandTable final
    {
        friend class AutoMcpServer;

        AutoMcpCommandTable() = delete;

        [[nodiscard]] static const std::unordered_map<std::string, AutoMcpCommand>& Get();
    };
}
