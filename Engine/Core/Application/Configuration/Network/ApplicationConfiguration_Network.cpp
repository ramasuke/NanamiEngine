#include "ApplicationConfiguration_Network.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    Network::Mode             NetworkConfiguration::mode_             = Network::Mode::Client;
    Network::ServerType       NetworkConfiguration::serverType_       = Network::ServerType::Relay;
    Network::ConnectionTarget NetworkConfiguration::connectionTarget_ = Network::ConnectionTarget::Localhost;
    std::string               NetworkConfiguration::lanAddress_       = "192.168.0.1";

    constexpr auto NETWORK_CONFIG_PATH          = "Network/";
    constexpr auto NETWORK_CONFIG_KEY           = "NetworkMode";
    constexpr auto NETWORK_SERVER_TYPE_KEY      = "ServerType";
    constexpr auto NETWORK_CONNECTION_TARGET_KEY = "ConnectionTarget";
    constexpr auto NETWORK_LAN_ADDRESS_KEY      = "LanAddress";

    void NetworkConfiguration::Load()
    {
        mode_ = Module::ProjectConfig::LoadOrDefaultWithPath<Network::Mode>(
            NETWORK_CONFIG_PATH, NETWORK_CONFIG_KEY, Network::Mode::Client);
        serverType_ = Module::ProjectConfig::LoadOrDefaultWithPath<Network::ServerType>(
            NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, Network::ServerType::Relay);
        connectionTarget_ = Module::ProjectConfig::LoadOrDefaultWithPath<Network::ConnectionTarget>(
            NETWORK_CONFIG_PATH, NETWORK_CONNECTION_TARGET_KEY, Network::ConnectionTarget::Localhost);
        lanAddress_ = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(
            NETWORK_CONFIG_PATH, NETWORK_LAN_ADDRESS_KEY, std::string("192.168.0.1"));
    }

    void NetworkConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<Network::Mode>(
            NETWORK_CONFIG_PATH, NETWORK_CONFIG_KEY, mode_);
        Module::ProjectConfig::SaveWithPath<Network::ServerType>(
            NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, serverType_);
        Module::ProjectConfig::SaveWithPath<Network::ConnectionTarget>(
            NETWORK_CONFIG_PATH, NETWORK_CONNECTION_TARGET_KEY, connectionTarget_);
        Module::ProjectConfig::SaveWithPath<std::string>(
            NETWORK_CONFIG_PATH, NETWORK_LAN_ADDRESS_KEY, lanAddress_);
    }

    bool NetworkConfiguration::IsServer() { return mode_ == Network::Mode::Server; }

    Network::Mode NetworkConfiguration::GetMode() { return mode_; }
    void          NetworkConfiguration::SetMode(const Network::Mode mode) { mode_ = mode; }

    Network::ServerType NetworkConfiguration::GetServerType() { return serverType_; }
    void                NetworkConfiguration::SetServerType(const Network::ServerType type) { serverType_ = type; }

    Network::ConnectionTarget NetworkConfiguration::GetConnectionTarget() { return connectionTarget_; }
    void                      NetworkConfiguration::SetConnectionTarget(const Network::ConnectionTarget target) { connectionTarget_ = target; }

    const std::string& NetworkConfiguration::GetLanAddress() { return lanAddress_; }
    void               NetworkConfiguration::SetLanAddress(const std::string& address) { lanAddress_ = address; }

    const char* NetworkConfiguration::GetServerAddress()
    {
        if (connectionTarget_ == Network::ConnectionTarget::Localhost)
            return "127.0.0.1";
        return lanAddress_.c_str();
    }

    void NetworkConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Network Mode");
        ImGui::Separator();

        int modeIndex = (mode_ == Network::Mode::Server) ? 0 : 1;
        if (ImGui::RadioButton("Server", &modeIndex, 0) || ImGui::RadioButton("Client", &modeIndex, 1))
        {
            mode_ = (modeIndex == 0) ? Network::Mode::Server : Network::Mode::Client;
            Save();
        }

        if (mode_ == Network::Mode::Server)
        {
            ImGui::Separator();
            ImGui::Text("Server Type");

            int serverTypeIndex = (serverType_ == Network::ServerType::Relay) ? 0 : 1;
            if (ImGui::RadioButton("Relay", &serverTypeIndex, 0) ||
                ImGui::RadioButton("Authoritative", &serverTypeIndex, 1))
            {
                serverType_ = (serverTypeIndex == 0)
                    ? Network::ServerType::Relay
                    : Network::ServerType::Authoritative;
                Save();
            }
        }

        if (mode_ == Network::Mode::Client)
        {
            ImGui::Spacing();
            ImGui::Text("Connection Target");
            ImGui::Separator();

            int targetIndex = (connectionTarget_ == Network::ConnectionTarget::Localhost) ? 0 : 1;
            if (ImGui::RadioButton("Localhost", &targetIndex, 0) || ImGui::RadioButton("LAN", &targetIndex, 1))
            {
                connectionTarget_ = (targetIndex == 0)
                    ? Network::ConnectionTarget::Localhost
                    : Network::ConnectionTarget::LAN;
                Save();
            }

            if (connectionTarget_ == Network::ConnectionTarget::LAN)
            {
                char lanBuf[64] = {};
                snprintf(lanBuf, sizeof(lanBuf), "%s", lanAddress_.c_str());
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputText("Server IP", lanBuf, sizeof(lanBuf)))
                {
                    lanAddress_ = lanBuf;
                    Save();
                }
            }
        }
    }
}
