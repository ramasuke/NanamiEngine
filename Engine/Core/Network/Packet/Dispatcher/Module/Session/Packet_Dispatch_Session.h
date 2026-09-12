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
     * セッション管理パケット(PlayerLeft / OwnershipSnapshot)のディスパッチャ。
     * - PlayerLeft: ホストが DISCONNECT を検知して全員へ配る(自分の受信キューにも積む)。
     *   全ピアが同じ手順で離脱者のアバターを破棄し、残りの所有物の所有者をホストへ付け替える。
     * - OwnershipSnapshot: ホストが新規参加者へ「Spawn したピア ≠ 現在の所有者」の一覧を送る。
     * ReceivePacket を直接オーバーライドしているので、Relay サーバーでも再ブロードキャストしない。
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

        /** 離脱処理(アバター破棄・所有権移譲)が終わった後に、離脱した PlayerId を通知する。ゲーム側の UI 掃除用 */
        [[nodiscard]] rxcpp::observable<struct PlayerId> OnPlayerLeft() const { return onPlayerLeft_.get_observable(); }

    private:
        void OnPlayerLeftReceived(const Packet& packet);
        void OnOwnershipSnapshotReceived(const Packet& packet);
        /** 全ピアで同一の手順: left の所有物を OwnerLeavePolicy に従って破棄 or newOwner へ移譲する */
        void ApplyPlayerLeft(struct PlayerId left, struct PlayerId newOwner);
        void SendOwnershipSnapshotTo(_ENetPeer* peer) const;

        INetworkSystem& networkSystem_;
        INetworkObjectInstanceRegistry& instanceRegistry_;
        SyncTransformDispatcher& syncTransform_;
        rxcpp::subjects::subject<struct PlayerId> onPlayerLeft_;
        rxcpp::composite_subscription newPeerSubscription_;
    };
}
