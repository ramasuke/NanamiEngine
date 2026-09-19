#include "ApplicationConfiguration_Network.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    Network::ServerType       NetworkConfiguration::serverType_         = Network::ServerType::Relay;
    int                       NetworkConfiguration::maxClients_         = 32;
    int                       NetworkConfiguration::unreliableSendRate_ = 20;

    constexpr auto NETWORK_CONFIG_PATH              = "Network/";
    constexpr auto NETWORK_SERVER_TYPE_KEY          = "ServerType";
    constexpr auto NETWORK_MAX_CLIENTS_KEY          = "MaxClients";
    constexpr auto NETWORK_UNRELIABLE_SEND_RATE_KEY = "UnreliableSendRate";

    void NetworkConfiguration::Load()
    {
        serverType_         = Module::ProjectConfig::LoadOrDefaultWithPath<Network::ServerType>(NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, Network::ServerType::Relay);
        maxClients_         = Module::ProjectConfig::LoadOrDefaultWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_MAX_CLIENTS_KEY, 32);
        unreliableSendRate_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_UNRELIABLE_SEND_RATE_KEY, 20);
    }

    void NetworkConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<Network::ServerType>(NETWORK_CONFIG_PATH, NETWORK_SERVER_TYPE_KEY, serverType_);
        Module::ProjectConfig::SaveWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_MAX_CLIENTS_KEY, maxClients_);
        Module::ProjectConfig::SaveWithPath<int>(NETWORK_CONFIG_PATH, NETWORK_UNRELIABLE_SEND_RATE_KEY, unreliableSendRate_);
    }

    Network::ServerType NetworkConfiguration::GetServerType() { return serverType_; }
    void                NetworkConfiguration::SetServerType(const Network::ServerType type) { serverType_ = type; }

    int  NetworkConfiguration::GetMaxClients() { return maxClients_; }
    void NetworkConfiguration::SetMaxClients(const int maxClients) { maxClients_ = maxClients; }

    int  NetworkConfiguration::GetUnreliableSendRate() { return unreliableSendRate_; }
    void NetworkConfiguration::SetUnreliableSendRate(const int hz) { unreliableSendRate_ = hz; }

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
    }
}
