#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../PlayerId/PlayerId.h"
#include "../../../../../../../Packages/R4/R4.h"

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
        [[nodiscard]] R4::Observable<struct PlayerId> OnPlayerLeft() const { return onPlayerLeft_.AsObservable(); }

    private:
        void OnPlayerLeftReceived(const Packet& packet);
        void OnOwnershipSnapshotReceived(const Packet& packet);
        void ApplyPlayerLeft(struct PlayerId left, struct PlayerId newOwner);
        void SendOwnershipSnapshotTo(struct PlayerId target) const;

        INetworkSystem& networkSystem_;
        INetworkObjectInstanceRegistry& instanceRegistry_;
        SyncTransformDispatcher& syncTransform_;
        R4::Subject<struct PlayerId> onPlayerLeft_;
        R4::Disposable newPeerSubscription_;
    };
}
