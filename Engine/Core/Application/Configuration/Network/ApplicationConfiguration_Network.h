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

        [[nodiscard]] static bool               IsServer();
        [[nodiscard]] static Network::Mode      GetMode();
        static void                             SetMode(Network::Mode mode);

        [[nodiscard]] static Network::ServerType  GetServerType();
        static void                               SetServerType(Network::ServerType type);

        [[nodiscard]] static Network::ConnectionTarget GetConnectionTarget();
        static void                                    SetConnectionTarget(Network::ConnectionTarget target);

        [[nodiscard]] static const std::string& GetLanAddress();
        static void                             SetLanAddress(const std::string& address);

        [[nodiscard]] static const char*        GetServerAddress();

        static void DrawConfigGUI();

    private:
        static Network::Mode             mode_;
        static Network::ServerType       serverType_;
        static Network::ConnectionTarget connectionTarget_;
        static std::string               lanAddress_;
    };
}