#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../ObjectId/Engine_Network_NetworkObjectId.h"
#include "../glm/vec3.hpp"
#include "../glm/detail/type_quat.hpp"
#include <deque>
#include <unordered_map>
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
        void Update();

    protected:
        void OnReceive(const Packet& packet) override;

    private:
        struct Snapshot
        {
            float     receiveTime;
            glm::vec3 position;
            glm::quat rotation;
        };

        static constexpr size_t kMaxBufferSize = 16;

        INetworkObjectInstanceRegistry& instanceRegistry_;
        std::unordered_map<uint32_t, std::deque<Snapshot>> snapshotBuffer_;
    };
}
