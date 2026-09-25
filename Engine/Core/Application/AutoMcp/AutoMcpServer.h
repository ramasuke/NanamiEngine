#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "AutoMcpCommands.h"
#include "AutoMcpScreenshot.h"

namespace NanamiEngine::Core::Application
{
    class EditorApplication;
}

namespace NanamiEngine::Core::Application::Configuration
{
    class AutoMcpConfiguration;
}

namespace NanamiEngine::Core::Application::AutoMcp
{
    /**
     * @brief tools/automcp (MCP ブリッジ) からの要求を 127.0.0.1 の TCP で受ける。
     * 1 行 1 JSON の要求 {"id","cmd","args"} に {"id","ok","result"|"error"} を 1 行で返す。
     * スレッドは使わず、メインループのフックから毎フレームポーリングする。
     * フックを呼ぶ EditorApplication と、設定画面の AutoMcpConfiguration からだけ触れる。
     */
    class NANAMI_API AutoMcpServer final
    {
        friend class ::NanamiEngine::Core::Application::EditorApplication;
        friend class ::NanamiEngine::Core::Application::Configuration::AutoMcpConfiguration;

        static AutoMcpServer& Instance();

        /** @brief AutoMcpConfiguration に合わせて待ち受けを開始・停止する。ポートが変わっていれば張り直す */
        void ApplyConfiguration();
        void Stop();

        /** @brief ImGui::NewFrame 直後に呼ぶ */
        void OnFrameBegin();
        /** @brief 3D 描画 (RenderVertex) 後、ImGui 描画前に呼ぶ */
        void OnSceneRendered();
        /** @brief ImGui 描画後、ScreenFlip 前に呼ぶ */
        void OnFrameEnd();

        [[nodiscard]] bool               IsListening()   const;
        [[nodiscard]] int                ListeningPort() const { return listeningPort_; }
        [[nodiscard]] int                ClientCount()   const { return static_cast<int>(clients_.size()); }
        [[nodiscard]] const std::string& LastError()     const { return lastError_; }

        struct NANAMI_API Client
        {
            std::uint64_t  id = 0;
            std::uintptr_t socket = 0;
            std::string    receiveBuffer;
            std::string    sendBuffer;
            bool           isBusy    = false;
            bool           isClosing = false;
        };

        struct NANAMI_API PendingRequest
        {
            std::uint64_t                 clientId  = 0;
            std::int64_t                  requestId = -1;
            std::string                   command;
            std::shared_ptr<JsonDocument> document;
        };

        struct NANAMI_API PendingScreenshot
        {
            PendingRequest request;
            bool           isGameOnly = false;
            bool           isGrabbed  = false;
            AutoMcpCapture capture;
        };

        AutoMcpServer();
        AutoMcpServer(const AutoMcpServer&) = delete;
        AutoMcpServer& operator=(const AutoMcpServer&) = delete;

        bool Start(int port);
        void AcceptClients();
        void ReceiveFromClients();
        void DispatchLines(Client& client);
        void DispatchLine(Client& client, const std::string& line);
        void Execute(const PendingRequest& request, const AutoMcpCommand& command);
        void RunFrameBeginQueue();
        void GrabScreenshots(bool isGameOnly);
        void FinishScreenshots();
        void SendResult(const PendingRequest& request, const JsonValue& result);
        void SendError(const PendingRequest& request, const std::string& message);
        void Send(const PendingRequest& request, JsonDocument& response);
        void Flush(Client& client) const;
        void RemoveClosedClients();
        [[nodiscard]] Client* FindClient(std::uint64_t id) const;
        [[nodiscard]] static JsonArgs Args(const PendingRequest& request);

        std::uintptr_t                        listenSocket_;
        int                                   listeningPort_    = 0;
        bool                                  isWinsockStarted_ = false;
        std::uint64_t                         nextClientId_     = 1;
        std::vector<std::unique_ptr<Client>>  clients_;
        std::deque<PendingRequest>            frameBeginQueue_;
        std::vector<PendingScreenshot>        screenshots_;
        std::vector<char>                     receiveChunk_;
        std::string                           lastError_;
    };
}
