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

    void NetworkRunnerBase::OnDrawGui()
    {
    }
}
