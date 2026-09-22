#include "Packet_Dispatch_Session.h"

#include "cereal/types/vector.hpp"
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
        // 新規参加者へ所有者テーブルを送る
        newPeerSubscription_ = networkSystem_.OnConnectPlayer().Subscribe(
            [this](const struct PlayerId joined)
            {
                if (IsServer())
                    SendOwnershipSnapshotTo(joined);
            });
    }

    SessionDispatcher::~SessionDispatcher()
    {
        newPeerSubscription_.Dispose();
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
        const auto owners = packet.Data().Read<std::vector<ObjectOwner>>(offset);
        for (const auto& [id, owner] : owners)
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
                //破棄するべきオブジェクト破棄
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

        onPlayerLeft_.OnNext(left);
    }

    void SessionDispatcher::SendOwnershipSnapshotTo(const struct PlayerId target) const
    {
        const auto owners = instanceRegistry_.CollectOwners();
        if (owners.empty())
            return;

        Packet packet = Packet::Create(DefaultPacketType::OwnershipSnapshot);
        packet.Data().Write(owners);
        networkSystem_.SendTo(target, packet);
    }
}
