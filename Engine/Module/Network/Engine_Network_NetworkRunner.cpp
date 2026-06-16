#include "Engine_Network_NetworkRunner.h"

#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "../../Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"

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

    void NetworkRunnerBase::Initialize()
    {
        Core::Application::Configuration::NetworkConfiguration::Load();
        networkSystem_ = DoCreateUseNetworkSystem();
        defaultPacketDispatcher_.emplace(*networkSystem_, networkSystem_->GetInstanceRegistry());
        DoInitialize();
    }

    Core::Network::PlayerId NetworkRunnerBase::GetPlayerId() const
    {
        return PlayerIdProvider().GetPlayerId();
    }

    void NetworkRunnerBase::OnUpdate()
    {
        networkSystem_->Update();
        DispatchPollPackets();
    }

    void NetworkRunnerBase::DispatchPollPackets()
    {
        const auto packets = networkSystem_->PollPackets();
        for (auto& packet : packets)
        {
            defaultPacketDispatcher_->DispatchReceivedPacket(packet);
            DoDispatchReceivedPacket(packet);
        }
    }

    void NetworkRunnerBase::SendNetworkPacket(const Core::Network::Packet& packet)
    {
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
            ImGui::Text(("playerId: " + networkSystem_->GetPlayerId().ToString()).c_str());
        }
        ImGuiHelper::OnDrawInputField("sampleSpawnPrefab_", sampleSpawnPrefab_);
        if (ImGui::Button("Sample Spawn Prefab"))
        {
            Spawn(*sampleSpawnPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f), glm::quat());
        }
    }
    
    void NetworkRunnerBase::Spawn(Asset::PrefabGameObjectFile& prefabFile, const glm::vec3 position, const glm::quat rotation)
    {
        defaultPacketDispatcher_->Spawn().DispatchSendPacket(prefabFile, position, rotation);
    }

    Coroutine::Task<void> NetworkRunnerBase::OnConnectedAsync()
    {
        co_await Coroutine::WaitForObservable(defaultPacketDispatcher_->ReceivedAssignPlayerId().OnAssignedPlayerId());
    }
}
