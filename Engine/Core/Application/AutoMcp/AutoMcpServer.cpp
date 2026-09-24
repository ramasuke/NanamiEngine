#include "AutoMcpServer.h"

#include <algorithm>
#include <optional>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../Configuration/AutoMcp/ApplicationConfiguration_AutoMcp.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Module/SafeExecute/Engine_Module_SafeExecute.h"

#pragma comment(lib, "Ws2_32.lib")

namespace NanamiEngine::Core::Application::AutoMcp
{
    namespace
    {
        constexpr std::uintptr_t AUTO_MCP_INVALID_SOCKET  = static_cast<std::uintptr_t>(INVALID_SOCKET);
        constexpr std::size_t    AUTO_MCP_MAX_CLIENTS     = 8;
        constexpr std::size_t    AUTO_MCP_MAX_BUFFER_SIZE = 64 * 1024 * 1024;
        constexpr std::size_t    AUTO_MCP_RECEIVE_CHUNK   = 64 * 1024;
        constexpr int            AUTO_MCP_MAX_SEND_CHUNK  = 1024 * 1024;

        SOCKET ToSocket(const std::uintptr_t socket)
        {
            return static_cast<SOCKET>(socket);
        }

        void CloseSocket(const std::uintptr_t socket)
        {
            if (socket != AUTO_MCP_INVALID_SOCKET)
                closesocket(ToSocket(socket));
        }

        bool SetNonBlocking(const SOCKET socket)
        {
            u_long nonBlocking = 1;
            return ioctlsocket(socket, FIONBIO, &nonBlocking) == 0;
        }
    }

    AutoMcpServer& AutoMcpServer::Instance()
    {
        static AutoMcpServer server;
        return server;
    }

    AutoMcpServer::AutoMcpServer()
        : listenSocket_(AUTO_MCP_INVALID_SOCKET),
          receiveChunk_(AUTO_MCP_RECEIVE_CHUNK)
    {
    }

    bool AutoMcpServer::IsListening() const
    {
        return listenSocket_ != AUTO_MCP_INVALID_SOCKET;
    }

    void AutoMcpServer::ApplyConfiguration()
    {
        if (!Configuration::AutoMcpConfiguration::IsEnabled())
        {
            Stop();
            lastError_.clear();
            return;
        }

        const int port = Configuration::AutoMcpConfiguration::GetPort();
        if (IsListening() && listeningPort_ == port)
            return;

        Stop();
        Start(port);
    }

    bool AutoMcpServer::Start(const int port)
    {
        if (!isWinsockStarted_)
        {
            WSADATA data;
            if (const int error = WSAStartup(MAKEWORD(2, 2), &data); error != 0)
            {
                lastError_ = "WSAStartup failed (error " + std::to_string(error) + ")";
                return false;
            }
            isWinsockStarted_ = true;
        }

        const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET)
        {
            lastError_ = "socket() failed (WSA error " + std::to_string(WSAGetLastError()) + ")";
            return false;
        }

        // 他プロセスに同じポートを横取りされないようにする
        int exclusive = 1;
        setsockopt(listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));

        sockaddr_in address{};
        address.sin_family      = AF_INET;
        address.sin_port        = htons(static_cast<u_short>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(listenSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
        {
            lastError_ = "bind 127.0.0.1:" + std::to_string(port) + " failed (WSA error " + std::to_string(WSAGetLastError()) + "). Is another NanamiEngine using the port?";
            closesocket(listenSocket);
            Module::LogWarning("AutoMCP: " + lastError_);
            return false;
        }

        if (listen(listenSocket, 4) == SOCKET_ERROR || !SetNonBlocking(listenSocket))
        {
            lastError_ = "listen() failed (WSA error " + std::to_string(WSAGetLastError()) + ")";
            closesocket(listenSocket);
            Module::LogWarning("AutoMCP: " + lastError_);
            return false;
        }

        listenSocket_  = static_cast<std::uintptr_t>(listenSocket);
        listeningPort_ = port;
        lastError_.clear();
        Module::Log("AutoMCP: 127.0.0.1:" + std::to_string(port) + " で待ち受けを開始しました");
        return true;
    }

    void AutoMcpServer::Stop()
    {
        const bool wasListening = IsListening();

        for (const auto& client : clients_)
            CloseSocket(client->socket);
        clients_.clear();

        CloseSocket(listenSocket_);
        listenSocket_  = AUTO_MCP_INVALID_SOCKET;
        listeningPort_ = 0;

        frameBeginQueue_.clear();
        for (auto& screenshot : screenshots_)
            AutoMcpScreenshot::Release(screenshot.capture);
        screenshots_.clear();

        if (isWinsockStarted_)
        {
            WSACleanup();
            isWinsockStarted_ = false;
        }

        if (wasListening)
            Module::Log("AutoMCP: 待ち受けを停止しました");
    }

    void AutoMcpServer::OnFrameBegin()
    {
        if (!IsListening())
            return;

        RunFrameBeginQueue();
    }

    void AutoMcpServer::OnSceneRendered()
    {
        if (!IsListening())
            return;

        GrabScreenshots(true);
    }

    void AutoMcpServer::OnFrameEnd()
    {
        if (!IsListening())
            return;

        // 前フレームまでに受けたスクリーンショットを先に仕上げる。今フレームで受けた要求は次のフレームの絵を返す
        GrabScreenshots(false);
        FinishScreenshots();

        AcceptClients();
        ReceiveFromClients();
        for (const auto& client : clients_)
            DispatchLines(*client);

        for (const auto& client : clients_)
            Flush(*client);
        RemoveClosedClients();
    }

    void AutoMcpServer::AcceptClients()
    {
        while (true)
        {
            const SOCKET socket = accept(ToSocket(listenSocket_), nullptr, nullptr);
            if (socket == INVALID_SOCKET)
                return;

            if (clients_.size() >= AUTO_MCP_MAX_CLIENTS || !SetNonBlocking(socket))
            {
                closesocket(socket);
                continue;
            }

            int noDelay = 1;
            setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

            auto client = std::make_unique<Client>();
            client->id     = nextClientId_++;
            client->socket = static_cast<std::uintptr_t>(socket);
            clients_.push_back(std::move(client));
        }
    }

    void AutoMcpServer::ReceiveFromClients()
    {
        for (const auto& client : clients_)
        {
            while (!client->isClosing)
            {
                const int received = recv(ToSocket(client->socket), receiveChunk_.data(), static_cast<int>(receiveChunk_.size()), 0);
                if (received > 0)
                {
                    client->receiveBuffer.append(receiveChunk_.data(), static_cast<std::size_t>(received));
                    if (client->receiveBuffer.size() > AUTO_MCP_MAX_BUFFER_SIZE)
                    {
                        Module::LogWarning("AutoMCP: 受信データが大きすぎるため接続を切りました");
                        client->isClosing = true;
                    }
                    continue;
                }

                if (received == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
                    break;

                client->isClosing = true;
            }
        }
    }

    void AutoMcpServer::DispatchLines(Client& client)
    {
        while (!client.isBusy && !client.isClosing)
        {
            const std::size_t newline = client.receiveBuffer.find('\n');
            if (newline == std::string::npos)
                return;

            std::string line = client.receiveBuffer.substr(0, newline);
            client.receiveBuffer.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (!line.empty())
                DispatchLine(client, line);
        }
    }

    void AutoMcpServer::DispatchLine(Client& client, const std::string& line)
    {
        PendingRequest request;
        request.clientId = client.id;
        request.document = std::make_shared<JsonDocument>();
        request.document->Parse(line.c_str());

        if (request.document->HasParseError() || !request.document->IsObject())
        {
            SendError(request, "request is not a JSON object");
            return;
        }

        if (const JsonValue* id = JsonArgs(*request.document).FindMember("id"); id != nullptr && id->IsInt64())
            request.requestId = id->GetInt64();

        const JsonValue* command = JsonArgs(*request.document).FindMember("cmd");
        if (command == nullptr || !command->IsString())
        {
            SendError(request, "request has no \"cmd\" string");
            return;
        }
        request.command.assign(command->GetString(), command->GetStringLength());

        if (request.command == "screenshot")
        {
            const JsonValue* mode = Args(request).FindMember("mode");
            const std::string modeText = mode != nullptr && mode->IsString() ? mode->GetString() : "full";
            if (modeText != "full" && modeText != "game")
            {
                SendError(request, "screenshot mode must be \"full\" or \"game\"");
                return;
            }

            PendingScreenshot screenshot;
            screenshot.request    = request;
            screenshot.isGameOnly = modeText == "game";
            screenshots_.push_back(std::move(screenshot));
            client.isBusy = true;
            return;
        }

        const auto& commands = AutoMcpCommandTable::Get();
        const auto it = commands.find(request.command);
        if (it == commands.end())
        {
            SendError(request, "unknown command: " + request.command);
            return;
        }

        if (it->second.phase == AutoMcpPhase::FrameBegin)
        {
            frameBeginQueue_.push_back(request);
            client.isBusy = true;
            return;
        }

        Execute(request, it->second);
    }

    void AutoMcpServer::Execute(const PendingRequest& request, const AutoMcpCommand& command)
    {
        JsonDocument result(rapidjson::kObjectType);
        std::optional<std::string> exceptionMessage;
        std::string sehMessage;

        const bool isSucceeded = Module::SafeExecutor::Execute([&]
        {
            try
            {
                command.handler(Args(request), result, result.GetAllocator());
            }
            catch (const std::exception& exception)
            {
                exceptionMessage = exception.what();
            }
        }, sehMessage);

        if (exceptionMessage)
            SendError(request, exceptionMessage->empty() ? "command failed" : *exceptionMessage);
        else if (!isSucceeded)
            SendError(request, sehMessage.empty() ? "command crashed" : sehMessage);
        else
            SendResult(request, result);
    }

    void AutoMcpServer::RunFrameBeginQueue()
    {
        const auto& commands = AutoMcpCommandTable::Get();
        while (!frameBeginQueue_.empty())
        {
            const PendingRequest request = std::move(frameBeginQueue_.front());
            frameBeginQueue_.pop_front();

            if (FindClient(request.clientId) == nullptr)
                continue;

            Execute(request, commands.at(request.command));
        }
    }

    void AutoMcpServer::GrabScreenshots(const bool isGameOnly)
    {
        for (auto& screenshot : screenshots_)
        {
            if (screenshot.isGameOnly != isGameOnly || screenshot.isGrabbed)
                continue;

            screenshot.capture   = AutoMcpScreenshot::Grab();
            screenshot.isGrabbed = true;
        }
    }

    void AutoMcpServer::FinishScreenshots()
    {
        // game モードは OnSceneRendered を一度通ってから仕上げる
        std::vector<PendingScreenshot> remaining;
        for (auto& screenshot : screenshots_)
        {
            if (!screenshot.isGrabbed && FindClient(screenshot.request.clientId) != nullptr)
            {
                remaining.push_back(std::move(screenshot));
                continue;
            }

            if (FindClient(screenshot.request.clientId) != nullptr)
            {
                const JsonArgs args = Args(screenshot.request);
                const std::string format   = args.OptionalString("format", "jpeg");
                const int         maxWidth = args.OptionalInt("maxWidth", Configuration::AutoMcpConfiguration::GetScreenshotMaxWidth());
                const int         quality  = args.OptionalInt("quality", 85);

                JsonDocument result(rapidjson::kObjectType);
                std::optional<std::string> error;
                try
                {
                    AutoMcpScreenshot::Save(screenshot.capture, format, maxWidth > 0 ? maxWidth : Configuration::AutoMcpConfiguration::GetScreenshotMaxWidth(), quality, result, result.GetAllocator());
                    result.AddMember("mode", MakeString(screenshot.isGameOnly ? "game" : "full", result.GetAllocator()), result.GetAllocator());
                }
                catch (const std::exception& exception)
                {
                    error = exception.what();
                }

                if (error)
                    SendError(screenshot.request, *error);
                else
                    SendResult(screenshot.request, result);
            }

            AutoMcpScreenshot::Release(screenshot.capture);
        }
        screenshots_ = std::move(remaining);
    }

    void AutoMcpServer::SendResult(const PendingRequest& request, const JsonValue& result)
    {
        JsonDocument response(rapidjson::kObjectType);
        auto& allocator = response.GetAllocator();
        response.AddMember("ok", true, allocator);
        response.AddMember("result", JsonValue(result, allocator), allocator);
        Send(request, response);
    }

    void AutoMcpServer::SendError(const PendingRequest& request, const std::string& message)
    {
        JsonDocument response(rapidjson::kObjectType);
        auto& allocator = response.GetAllocator();
        response.AddMember("ok", false, allocator);
        response.AddMember("error", MakeString(message, allocator), allocator);
        Send(request, response);
    }

    void AutoMcpServer::Send(const PendingRequest& request, JsonDocument& response)
    {
        Client* client = FindClient(request.clientId);
        if (client == nullptr)
            return;

        auto& allocator = response.GetAllocator();
        if (request.requestId >= 0)
            response.AddMember("id", request.requestId, allocator);
        else
            response.AddMember("id", JsonValue(rapidjson::kNullType), allocator);

        client->sendBuffer += ToJsonText(response);
        client->sendBuffer.push_back('\n');
        client->isBusy = false;
        Flush(*client);
    }

    void AutoMcpServer::Flush(Client& client) const
    {
        while (!client.sendBuffer.empty() && !client.isClosing)
        {
            const int length = static_cast<int>(std::min<std::size_t>(client.sendBuffer.size(), AUTO_MCP_MAX_SEND_CHUNK));
            const int sent   = send(ToSocket(client.socket), client.sendBuffer.data(), length, 0);
            if (sent > 0)
            {
                client.sendBuffer.erase(0, static_cast<std::size_t>(sent));
                continue;
            }

            if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
                return;

            client.isClosing = true;
        }
    }

    void AutoMcpServer::RemoveClosedClients()
    {
        std::erase_if(clients_, [](const std::unique_ptr<Client>& client)
        {
            if (!client->isClosing)
                return false;

            CloseSocket(client->socket);
            return true;
        });
    }

    AutoMcpServer::Client* AutoMcpServer::FindClient(const std::uint64_t id) const
    {
        for (const auto& client : clients_)
        {
            if (client->id == id && !client->isClosing)
                return client.get();
        }
        return nullptr;
    }

    JsonArgs AutoMcpServer::Args(const PendingRequest& request)
    {
        static const JsonValue emptyObject(rapidjson::kObjectType);
        const JsonValue* args = request.document ? JsonArgs(*request.document).FindMember("args") : nullptr;
        return JsonArgs(args != nullptr && args->IsObject() ? *args : emptyObject);
    }
}
