#pragma once
#include "../../../Network/Mode/NetworkSystem_Mode.h"

namespace NanamiEngine::Core::Application::Configuration
{
    class NetworkConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static bool               IsServer();
        [[nodiscard]] static Network::Mode      GetMode();
        static void                             SetMode(Network::Mode mode);

        [[nodiscard]] static Network::ServerType GetServerType();
        static void                              SetServerType(Network::ServerType type);

    private:
        static Network::Mode       mode_;
        static Network::ServerType serverType_;
    };
}