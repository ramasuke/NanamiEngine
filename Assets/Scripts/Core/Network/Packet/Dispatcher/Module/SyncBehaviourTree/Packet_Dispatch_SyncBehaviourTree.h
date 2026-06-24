#pragma once
#include "../../CustomPacketDispatcherBase.h"
#include "../../../../../../../../Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../../../../../Engine/Core/Network/Packet/ByteBuffer/Packet_ByteBuffer.h"

namespace GameCore::Network
{
    class SyncBehaviourTreeDispatcher final : public CustomDispatcherBase
    {
    public:
        explicit SyncBehaviourTreeDispatcher(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender) {}

        void DispatchSendPacket(
            Core::Network::NetworkObjectId id,
            const Core::Network::ByteBuffer& paramBuffer);

        void OnReceive(const Core::Network::Packet& packet) override;
    };
}
