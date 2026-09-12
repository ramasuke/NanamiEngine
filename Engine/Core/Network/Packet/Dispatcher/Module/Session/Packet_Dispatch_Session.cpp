#include "Packet_Dispatch_Session.h"

#include "cereal/types/vector.hpp"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../../../../Engine_Network_INetworkSystem.h"
#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../SyncTransform/Packet_Dispatch_SyncTransform.h"
#include "../../../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Network
{
    SessionDispatcher::SessionDispatcher(
        INetworkSystem& networkSystem,
        INetworkObjectInstanceRegistry& instanceRegistry,
        SyncTransformDispatcher& syncTransform)
        : PacketDispatcherBase(networkSystem, networkSystem)
        , networkSystem_(networkSystem)
        , instanceRegistry_(instanceRegistry)
        , syncTransform_(syncTransform)
    {
        // 新規参加者へ所有者の上書き一覧を送る(ホストのみ)。
        // スポーン履歴の再送と到着順が前後しても、レジストリ側が SetOwner 先着を保持するので問題ない
        newPeerSubscription_ = networkSystem_.OnConnectPlayer().subscribe(
            [this](const ENetEvent* event)
            {
                if (IsServer())
                    SendOwnershipSnapshotTo(event->peer);
            },
            [](std::exception_ptr) {}
        );
    }

    SessionDispatcher::~SessionDispatcher()
    {
        newPeerSubscription_.unsubscribe();
    }

    void SessionDispatcher::ReceivePacket(const Packet& packet)
    {
        // Relay サーバーでも再ブロードキャストしない(ホストは Send + 自分の受信キューで配っている)
        switch (static_cast<DefaultPacketType>(packet.Type()))
        {
        case DefaultPacketType::PlayerLeft:
            OnPlayerLeftReceived(packet);
            break;
        case DefaultPacketType::OwnershipSnapshot:
            OnOwnershipSnapshotReceived(packet);
            break;
        default:
            break;
        }
    }

    void SessionDispatcher::OnPlayerLeftReceived(const Packet& packet)
    {
        size_t offset = 0;
        const auto left     = packet.Data().Read<struct PlayerId>(offset);
        const auto newOwner = packet.Data().Read<struct PlayerId>(offset);
        ApplyPlayerLeft(left, newOwner);
    }

    void SessionDispatcher::OnOwnershipSnapshotReceived(const Packet& packet)
    {
        size_t offset = 0;
        const auto overrides = packet.Data().Read<std::vector<OwnerOverride>>(offset);
        for (const auto& [id, owner] : overrides)
            instanceRegistry_.SetOwner(id, owner);
    }

    void SessionDispatcher::ApplyPlayerLeft(const struct PlayerId left, const struct PlayerId newOwner)
    {
        const auto owned = instanceRegistry_.CollectOwnedBy(left);
        Module::Log("SessionDispatcher: player " + left.ToString() + " left. objects=" + std::to_string(owned.size())
            + " newOwner=" + newOwner.ToString());

        for (const auto& [id, policy] : owned)
        {
            // 受信済みの古いスナップショットに新所有者が引き戻されないよう捨てる
            syncTransform_.Forget(id);

            if (policy == OwnerLeavePolicy::Destroy)
            {
                // ルートだけ破棄すれば子は一緒に破棄される(ImplementDestroy が子へ再帰する)
                const auto object = instanceRegistry_.Find(id).lock();
                if (object && !object->Transform().GetParent())
                    object->OnDestroy();
                instanceRegistry_.Unregister(id);
            }
            else
            {
                instanceRegistry_.SetOwner(id, newOwner);
            }
        }

        onPlayerLeft_.get_subscriber().on_next(left);
    }

    void SessionDispatcher::SendOwnershipSnapshotTo(_ENetPeer* peer) const
    {
        const auto overrides = instanceRegistry_.CollectOwnerOverrides();
        if (overrides.empty())
            return;

        Packet packet = Packet::Create(DefaultPacketType::OwnershipSnapshot);
        packet.Data().Write(overrides);
        networkSystem_.SendTo(peer, packet);
    }
}
