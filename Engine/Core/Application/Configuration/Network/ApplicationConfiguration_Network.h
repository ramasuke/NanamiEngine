#pragma once
#include <string>

#include "../../../Network/Mode/NetworkSystem_Mode.h"

namespace NanamiEngine::Core::Application::Configuration
{
    class NetworkConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static Network::ServerType  GetServerType();
        static void                               SetServerType(Network::ServerType type);

        [[nodiscard]] static int  GetMaxClients();
        static void               SetMaxClients(int maxClients);

        [[nodiscard]] static int  GetUnreliableSendRate();
        static void               SetUnreliableSendRate(int hz);

        /** 中継サーバーを使う設定で、接続先と App Id が埋まっているか */
        [[nodiscard]] static bool IsRelayServerEnabled();
        [[nodiscard]] static Network::HostEndpoint GetRelayServerEndpoint();
        /** 中継サーバー上で他のゲームと部屋を分けるための名前 */
        [[nodiscard]] static const std::string& GetRelayAppId();

        static void DrawConfigGUI();

    private:
        static Network::ServerType       serverType_;
        static int                       maxClients_;
        static int                       unreliableSendRate_;
        static bool                      useRelayServer_;
        static std::string               relayServerAddress_;
        static int                       relayServerPort_;
        static std::string               relayAppId_;
    };
}
