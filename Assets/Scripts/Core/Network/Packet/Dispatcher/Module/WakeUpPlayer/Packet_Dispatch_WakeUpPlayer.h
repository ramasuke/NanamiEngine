#pragma once
#include "../../CustomPacketDispatcherBase.h"
#include "../../../../../../../../Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"

namespace GameCore::Network
{
    class WakeUpPlayerDispatcher final : public CustomDispatcherBase
    {
    public:
        explicit WakeUpPlayerDispatcher(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender) {}

        void DispatchSendPacket(Core::Network::NetworkObjectId targetId);

        void OnReceive(const Core::Network::Packet& packet) override;
    };
}
