#include "Engine_Network_NetworkRunner.h"

#include "../../Core/Network/Engine_Network_INetworkSystem.h"

namespace NanamiEngine::Module::Network
{
    Core::Network::DefaultPacketDispatcher& NetworkRunnerBase::DefaultDispatcher()
    {
        assert(defaultPacketDispatcher_, "defaultPacketDispatcher is null");
        return defaultPacketDispatcher_.value();
    }

    void NetworkRunnerBase::Initialize()
    {
        networkSystem_ = DoCreateUseNetworkSystem();
        defaultPacketDispatcher_.emplace(*networkSystem_);
        DoInitialize();
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
            DefaultDispatcher().Spawn().DispatchSendPacket(*sampleSpawnPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f), glm::quat());
        }
    }
}
