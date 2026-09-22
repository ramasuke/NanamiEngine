#include "Engine_Network_NetworkRunner.h"

#include <algorithm>
#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Network/Discovery/LanSessionAdvertiser.h"
#include "../../Core/Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../Exception/Engine_Module_Exception.h"
#include "../Log/NanamiEngine_Module_Log.h"

Network::NetworkRunnerBase* Network::NetworkRunnerBase::s_instance_ = nullptr;

namespace NanamiEngine::Module::Network
{
    NetworkRunnerBase::NetworkRunnerBase()
    {
        s_instance_ = this;
    }

    NetworkRunnerBase::~NetworkRunnerBase()
    {
        if (s_instance_ == this) s_instance_ = nullptr;
    }

    NetworkRunnerBase& NetworkRunnerBase::Instance()
    {
        assert(s_instance_ != nullptr);
        return *s_instance_;
    }

    Core::Network::DefaultPacketDispatcher& NetworkRunnerBase::DefaultDispatcher()
    {
        assert(defaultPacketDispatcher_, "defaultPacketDispatcher is null");
        return defaultPacketDispatcher_.value();
    }

    void NetworkRunnerBase::StartHost(const std::string& sessionKey)
    {
        Start({ Core::Network::Mode::Server, Core::Network::HostEndpoint{} });
        if (networkSystem_->GetConnectionState() != Core::Network::ConnectionState::Failed)
            lanAdvertiser_ = std::make_unique<Core::Network::LanSessionAdvertiser>(sessionKey, networkSystem_->ListenPort());
    }

    void NetworkRunnerBase::StartClient(const Core::Network::HostEndpoint& host)
    {
        Start({ Core::Network::Mode::Client, host });
    }

    void NetworkRunnerBase::StartRelay(const std::string& sessionKey)
    {
        // Start() でも読み直すが、接続先はここで決めるので先に読む
        Core::Application::Configuration::NetworkConfiguration::Load();
        Core::Network::NetworkStartSettings settings;
        settings.host       = Core::Application::Configuration::NetworkConfiguration::GetRelayServerEndpoint();
        settings.transport  = Core::Network::Transport::RelayServer;
        settings.sessionKey = sessionKey;
        Start(settings);
    }

    void NetworkRunnerBase::Start(const Core::Network::NetworkStartSettings& settings)
    {
        assert(!networkSystem_ && "NetworkRunner は開始済み。やり直すときは先に Shutdown する");

        Core::Application::Configuration::NetworkConfiguration::Load();
        networkSystem_ = DoCreateUseNetworkSystem(settings);
        defaultPacketDispatcher_.emplace(*networkSystem_, networkSystem_->GetInstanceRegistry());
        DoInitialize();
    }

    void NetworkRunnerBase::Shutdown()
    {
        //NOTE: ディスパッチャーは networkSystem_ を参照しているので先に消す
        lanAdvertiser_.reset();
        if (networkSystem_)
            DoShutdown();
        
        defaultPacketDispatcher_.reset();
        networkSystem_.reset();
    }

    bool NetworkRunnerBase::IsStarted() const
    {
        return static_cast<bool>(networkSystem_);
    }

    bool NetworkRunnerBase::IsServer() const
    {
        return networkSystem_ && networkSystem_->IsServer();
    }

    Core::Network::ConnectionState NetworkRunnerBase::GetConnectionState() const
    {
        assert(networkSystem_ && "NetworkRunner が開始されていない");
        return networkSystem_->GetConnectionState();
    }

    Core::Network::PlayerId NetworkRunnerBase::GetPlayerId() const
    {
        if (!networkSystem_)
            return Core::Network::PlayerId::Invalid();
        
        return PlayerIdProvider().GetPlayerId();
    }

    Core::Network::PlayerId NetworkRunnerBase::OwnerOf(const Core::Network::NetworkObjectId id) const
    {
        if (!networkSystem_)
            return Core::Network::PlayerId::Invalid();
        
        return networkSystem_->GetInstanceRegistry().OwnerOf(id);
    }

    bool NetworkRunnerBase::IsLocallyOwned(const Core::Network::NetworkObjectId id) const
    {
        const auto owner = OwnerOf(id);
        return owner != Core::Network::PlayerId::Invalid() && owner == GetPlayerId();
    }

    void NetworkRunnerBase::OnUpdate()
    {
        if (!networkSystem_)
            return;

        networkSystem_->Update();
        if (lanAdvertiser_)
        {
            lanAdvertiser_->Update();
        }
        DispatchPollPackets();
        defaultPacketDispatcher_->Update();
    }

    void NetworkRunnerBase::DispatchPollPackets()
    {
        const auto packets = networkSystem_->PollPackets();
        for (auto& packet : packets)
        {
            try
            {
                defaultPacketDispatcher_->DispatchReceivedPacket(packet);
                DoDispatchReceivedPacket(packet);
            }
            catch (const Exception::PacketDeserializeException& exception)
            {
                LogWarning("NetworkRunner: パケットの処理に失敗しました: " + std::string(exception.what()));
            }
        }
    }

    void NetworkRunnerBase::SendNetworkPacket(const Core::Network::Packet& packet)
    {
        if (!networkSystem_)
            return;
        
        PacketSender().Send(packet);
    }

    Core::Network::IPacketSender& NetworkRunnerBase::PacketSender() const
    {
        return *networkSystem_;
    }

    Core::Network::IPlayerIdProvider& NetworkRunnerBase::PlayerIdProvider() const
    {
        return *networkSystem_;
    }

    void NetworkRunnerBase::BasedOnDrawgui()
    {
        if (networkSystem_)
        {
            constexpr std::array<const char*, 4> CONNECTION_STATE_NAMES{ "Connecting", "Connected", "Failed", "Disconnected" };
            ImGui::Text("role: %s", networkSystem_->IsServer() ? "Host" : "Client");
            ImGui::Text("state: %s", CONNECTION_STATE_NAMES[static_cast<size_t>(networkSystem_->GetConnectionState())]);
            if (lanAdvertiser_)
            {
                ImGui::Text("session: %s%s", lanAdvertiser_->SessionKey().c_str(), lanAdvertiser_->IsListening() ? "" : " (not advertised)");
            }

            const auto localPlayerId = networkSystem_->GetPlayerId();
            ImGui::Text(("playerId: " + localPlayerId.ToString()).c_str());

            auto owners = networkSystem_->GetInstanceRegistry().CollectOwners();
            std::ranges::sort(owners, [](const auto& lhs, const auto& rhs)
            {
                if (lhs.owner != rhs.owner) return lhs.owner < rhs.owner;
                return lhs.id < rhs.id;
            });

            ImGui::Text("network objects: %d", static_cast<int>(owners.size()));
            for (const auto& [id, owner] : owners)
            {
                const bool isLocal = owner == localPlayerId;
                if (isLocal)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 1.0f, 0.45f, 1.0f));
                }
                ImGui::BulletText("id %s -> owner %s", id.ToString().c_str(), owner.ToString().c_str());
                if (isLocal)
                {
                    ImGui::PopStyleColor();
                }
            }
        }
        else
        {
            ImGui::Text("not started");
        }
        ImGuiHelper::OnDrawInputField("sampleSpawnPrefab_", sampleSpawnPrefab_);
        if (ImGui::Button("Sample Spawn Prefab"))
        {
            Spawn(*sampleSpawnPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f), glm::quat());
        }
    }
    
    void NetworkRunnerBase::Spawn(Asset::PrefabGameObjectFile& prefabFile, const glm::vec3 position, const glm::quat rotation)
    {
        if (!defaultPacketDispatcher_)
            return;
        
        defaultPacketDispatcher_->Spawn().DispatchSendPacket(prefabFile, position, rotation);
    }
}
