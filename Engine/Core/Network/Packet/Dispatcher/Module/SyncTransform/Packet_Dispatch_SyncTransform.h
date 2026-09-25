#pragma once
#include "Engine/Core/Api/NanamiApi.h"
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

    class NANAMI_API SyncTransformDispatcher final : public PacketDispatcherBase
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
        /** 受信済みスナップショットを捨てる(所有権が自分に移った／破棄されたオブジェクト用) */
        void Forget(NetworkObjectId id);

    protected:
        void OnReceive(const Packet& packet) override;

    private:
        struct NANAMI_API Snapshot
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
