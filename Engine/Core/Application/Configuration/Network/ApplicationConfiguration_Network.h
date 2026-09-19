#pragma once
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

        static void DrawConfigGUI();

    private:
        static Network::ServerType       serverType_;
        static int                       maxClients_;
        static int                       unreliableSendRate_;
    };
}
