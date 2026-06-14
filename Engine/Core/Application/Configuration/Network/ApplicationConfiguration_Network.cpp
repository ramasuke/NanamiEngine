#include "ApplicationConfiguration_Network.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"

namespace NanamiEngine::Core::Application::Configuration
{
    Network::Mode       NetworkConfiguration::mode_       = Network::Mode::Client;
    Network::ServerType NetworkConfiguration::serverType_ = Network::ServerType::Relay;

    constexpr auto NETWORK_CONFIG_PATH       = "Network/";
    constexpr auto NETWORK_CONFIG_KEY        = "NetworkMode";
    constexpr auto NETWORK_SERVER_TYPE_KEY   = "ServerType";

    void NetworkConfiguration::Load()
    {
        mode_ = Module::ProjectConfig::LoadOrDefaultWithPath<Network::Mode>(
            NETWORK_CONFIG_PATH, NETWORK_CONFIG_KEY, Network::Mode::Client);
        serverType_ = Module::ProjectConfig::LoadOrDefaultWithPath<Network::ServerType>(
            NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, Network::ServerType::Relay);
    }

    void NetworkConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<Network::Mode>(
            NETWORK_CONFIG_PATH, NETWORK_CONFIG_KEY, mode_);
        Module::ProjectConfig::SaveWithPath<Network::ServerType>(
            NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, serverType_);
    }

    bool NetworkConfiguration::IsServer() { return mode_ == Network::Mode::Server; }
    Network::Mode NetworkConfiguration::GetMode() { return mode_; }
    void NetworkConfiguration::SetMode(const Network::Mode mode) { mode_ = mode; }

    Network::ServerType NetworkConfiguration::GetServerType() { return serverType_; }
    void NetworkConfiguration::SetServerType(const Network::ServerType type) { serverType_ = type; }
}
