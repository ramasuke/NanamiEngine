#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../PlayerId/PlayerId.h"
#include "../rxcpp/rx.hpp"

struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    class INetworkSystem;
    class INetworkObjectInstanceRegistry;
    class SyncTransformDispatcher;
}

namespace NanamiEngine::Core::Network
{
    /**
     * セッション管理
     */
    class SessionDispatcher final : public PacketDispatcherBase
    {
    public:
        explicit SessionDispatcher(
            INetworkSystem& networkSystem,
            INetworkObjectInstanceRegistry& instanceRegistry,
            SyncTransformDispatcher& syncTransform);
        ~SessionDispatcher() override;

        void ReceivePacket(const Packet& packet) override;

        /** 離脱した PlayerId を通知する */
        [[nodiscard]] rxcpp::observable<struct PlayerId> OnPlayerLeft() const { return onPlayerLeft_.get_observable(); }

    private:
        void OnPlayerLeftReceived(const Packet& packet);
        void OnOwnershipSnapshotReceived(const Packet& packet);
        void ApplyPlayerLeft(struct PlayerId left, struct PlayerId newOwner);
        void SendOwnershipSnapshotTo(_ENetPeer* peer) const;

        INetworkSystem& networkSystem_;
        INetworkObjectInstanceRegistry& instanceRegistry_;
        SyncTransformDispatcher& syncTransform_;
        rxcpp::subjects::subject<struct PlayerId> onPlayerLeft_;
        rxcpp::composite_subscription newPeerSubscription_;
    };
}
