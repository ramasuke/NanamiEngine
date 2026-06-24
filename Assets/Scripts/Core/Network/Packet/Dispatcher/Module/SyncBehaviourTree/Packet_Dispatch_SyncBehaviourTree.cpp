#include "Packet_Dispatch_SyncBehaviourTree.h"

#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../GamePlay/Npc/Enemy/NetworkBehaviourTree/GamePlay_NetworkBehaviourTree.h"

namespace GameCore::Network
{
    void SyncBehaviourTreeDispatcher::DispatchSendPacket(
        const NanamiEngine::Core::Network::NetworkObjectId id,
        const NanamiEngine::Core::Network::ByteBuffer& paramBuffer)
    {
        NanamiEngine::Core::Network::Packet packet = NanamiEngine::Core::Network::Packet::Create(
            static_cast<NanamiEngine::Core::Network::PacketType>(EPacketType::SyncBehaviourTree));
        packet.Data().Write(id);
        packet.Data().Append(paramBuffer.Data(), paramBuffer.Size());
        packet.SetDelivery(NanamiEngine::Core::Network::DeliveryMode::Unreliable);
        SendPacket(packet);
    }

    void SyncBehaviourTreeDispatcher::OnReceive(const NanamiEngine::Core::Network::Packet& packet)
    {
        size_t offset = 0;
        const auto id = packet.Data().Read<NanamiEngine::Core::Network::NetworkObjectId>(offset);

        if (id.IsOwnerBy(PlayerId()))
            return;

        auto networkObjectWeak = DefaultDispatch().FindNetworkObject(id);
        auto networkObject = networkObjectWeak.lock();
        if (!networkObject)
            return;

        auto btWeak = networkObject->Components().Catch<GamePlay::Npc::Enemy::NetworkBehaviourTree>();
        auto bt = btWeak.lock();
        if (!bt)
            return;

        bt->ApplyReceivedBuffer(packet.Data(), offset);
    }
}
