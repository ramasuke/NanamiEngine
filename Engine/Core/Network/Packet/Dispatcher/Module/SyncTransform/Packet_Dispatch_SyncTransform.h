#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../ObjectId/Engine_Network_NetworkObjectId.h"
#include "../glm/fwd.hpp"

namespace NanamiEngine::Core::Network
{
    class INetworkObjectInstanceRegistry;

    class SyncTransformDispatcher final : public PacketDispatcherBase
    {
    public:
        explicit SyncTransformDispatcher(
            const IPlayerIdProvider& playerIdProvider,
            IPacketSender& packetSender,
            INetworkObjectInstanceRegistry& instanceRegistry)
            : PacketDispatcherBase(playerIdProvider, packetSender)
            , instanceRegistry_(instanceRegistry) {}

        void DispatchSendPacket(NetworkObjectId id, glm::vec3 position, glm::quat rotation);

    protected:
        void OnReceive(const Packet& packet) override;

    private:
        INetworkObjectInstanceRegistry& instanceRegistry_;
    };
}
