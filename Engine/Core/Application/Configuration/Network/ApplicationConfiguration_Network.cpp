#include "ApplicationConfiguration_Network.h"

#include <algorithm>
#include <cstdio>
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    Network::ServerType       NetworkConfiguration::serverType_         = Network::ServerType::Relay;
    int                       NetworkConfiguration::maxClients_         = 32;
    int                       NetworkConfiguration::unreliableSendRate_ = 20;
    bool                      NetworkConfiguration::useRelayServer_     = false;
    std::string               NetworkConfiguration::relayServerAddress_;
    int                       NetworkConfiguration::relayServerPort_    = 1234;
    std::string               NetworkConfiguration::relayAppId_;

    constexpr auto NETWORK_CONFIG_PATH              = "Network/";
    constexpr auto NETWORK_SERVER_TYPE_KEY          = "ServerType";
    constexpr auto NETWORK_MAX_CLIENTS_KEY          = "MaxClients";
    constexpr auto NETWORK_UNRELIABLE_SEND_RATE_KEY = "UnreliableSendRate";
    constexpr auto NETWORK_USE_RELAY_SERVER_KEY     = "UseRelayServer";
    constexpr auto NETWORK_RELAY_SERVER_ADDRESS_KEY = "RelayServerAddress";
    constexpr auto NETWORK_RELAY_SERVER_PORT_KEY    = "RelayServerPort";
    constexpr auto NETWORK_RELAY_APP_ID_KEY         = "RelayAppId";

    namespace
    {
        /** std::string を InputText で編集し、編集が確定したフレームで true を返す */
        bool InputString(const char* label, std::string& value)
        {
            char buffer[256] = {};
            snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if (ImGui::InputText(label, buffer, sizeof(buffer)))
                value = buffer;
            return ImGui::IsItemDeactivatedAfterEdit();
        }
    }

    void NetworkConfiguration::Load()
    {
        serverType_         = Module::ProjectConfig::LoadOrDefaultWithPath<Network::ServerType>(NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, Network::ServerType::Relay);
        maxClients_         = Module::ProjectConfig::LoadOrDefaultWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_MAX_CLIENTS_KEY, 32);
        unreliableSendRate_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_UNRELIABLE_SEND_RATE_KEY, 20);
        useRelayServer_     = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(NETWORK_CONFIG_PATH, NETWORK_USE_RELAY_SERVER_KEY, false);
        relayServerAddress_ = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(NETWORK_CONFIG_PATH, NETWORK_RELAY_SERVER_ADDRESS_KEY, std::string());
        relayServerPort_    = Module::ProjectConfig::LoadOrDefaultWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_RELAY_SERVER_PORT_KEY, 1234);
        relayAppId_         = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(NETWORK_CONFIG_PATH, NETWORK_RELAY_APP_ID_KEY, std::string());
    }

    void NetworkConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<Network::ServerType>(NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, serverType_);
        Module::ProjectConfig::SaveWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_MAX_CLIENTS_KEY, maxClients_);
        Module::ProjectConfig::SaveWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_UNRELIABLE_SEND_RATE_KEY, unreliableSendRate_);
        Module::ProjectConfig::SaveWithPath<bool>(NETWORK_CONFIG_PATH, NETWORK_USE_RELAY_SERVER_KEY, useRelayServer_);
        Module::ProjectConfig::SaveWithPath<std::string>(NETWORK_CONFIG_PATH, NETWORK_RELAY_SERVER_ADDRESS_KEY, relayServerAddress_);
        Module::ProjectConfig::SaveWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_RELAY_SERVER_PORT_KEY, relayServerPort_);
        Module::ProjectConfig::SaveWithPath<std::string>(NETWORK_CONFIG_PATH, NETWORK_RELAY_APP_ID_KEY, relayAppId_);
    }

    Network::ServerType NetworkConfiguration::GetServerType() { return serverType_; }
    void                NetworkConfiguration::SetServerType(const Network::ServerType type) { serverType_ = type; }

    int  NetworkConfiguration::GetMaxClients() { return maxClients_; }
    void NetworkConfiguration::SetMaxClients(const int maxClients) { maxClients_ = maxClients; }

    int  NetworkConfiguration::GetUnreliableSendRate() { return unreliableSendRate_; }
    void NetworkConfiguration::SetUnreliableSendRate(const int hz) { unreliableSendRate_ = hz; }

    bool NetworkConfiguration::IsRelayServerEnabled()
    {
        return useRelayServer_ && !relayServerAddress_.empty() && !relayAppId_.empty();
    }

    Network::HostEndpoint NetworkConfiguration::GetRelayServerEndpoint()
    {
        return { relayServerAddress_, static_cast<std::uint16_t>(relayServerPort_) };
    }

    const std::string& NetworkConfiguration::GetRelayAppId() { return relayAppId_; }

    void NetworkConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Host / Client is decided at runtime by the game.");
        ImGui::Separator();

        ImGui::Text("Server Type (host only)");
        int serverTypeIndex = (serverType_ == Network::ServerType::Relay) ? 0 : 1;
        if (ImGui::RadioButton("Relay", &serverTypeIndex, 0) ||
            ImGui::RadioButton("Authoritative", &serverTypeIndex, 1))
        {
            serverType_ = (serverTypeIndex == 0)
                ? Network::ServerType::Relay
                : Network::ServerType::Authoritative;
            Save();
        }

        ImGui::Spacing();
        ImGui::Text("Max Clients (host only)");
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##MaxClients", &maxClients_))
        {
            if (maxClients_ < 1) maxClients_ = 1;
            Save();
        }

        ImGui::Spacing();
        ImGui::Text("Unreliable Send Rate (Hz)");
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##UnreliableSendRate", &unreliableSendRate_))
        {
            if (unreliableSendRate_ < 1) unreliableSendRate_ = 1;
            Save();
        }

        ImGui::Separator();
        ImGui::Text("Internet Relay Server (EnviroHunter-Server)");
        if (ImGui::Checkbox("Use Relay Server", &useRelayServer_))
            Save();
        ImGui::SetNextItemWidth(200);
        if (InputString("Address", relayServerAddress_))
            Save();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Port", &relayServerPort_))
        {
            relayServerPort_ = std::clamp(relayServerPort_, 1, 65535);
            Save();
        }
        ImGui::SetNextItemWidth(200);
        if (InputString("App Id", relayAppId_))
            Save();
        if (useRelayServer_ && !IsRelayServerEnabled())
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Address and App Id are required (falls back to LAN)");
    }
}
