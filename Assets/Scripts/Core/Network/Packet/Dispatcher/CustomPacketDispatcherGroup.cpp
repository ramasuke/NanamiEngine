#include "CustomPacketDispatcherGroup.h"

#include "../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../Custom_PacketType.h"

namespace GameCore::Network
{
    CustomDispatcherGroup::CustomDispatcherGroup(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        Core::Network::IPacketSender& packetSender,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Asset::PlayerAvatarFactory& playerAvatarFactory)
        : spawnPlayerDispatcher_(defaultDispatchers, playerIdProvider, packetSender, playerAvatarFactory)
        , syncAvatarStateDispatcher_(defaultDispatchers, playerIdProvider, packetSender)
    {
    }

    void CustomDispatcherGroup::DispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        const auto customType = static_cast<EPacketType>(packet.Type());
        switch (customType)
        {
        case EPacketType::SpawnPlayerAvatar:
            spawnPlayerDispatcher_.ReceivePacket(packet);
            break;
        case EPacketType::SyncAvatarState:
            syncAvatarStateDispatcher_.ReceivePacket(packet);
            break;
        }
    }
}
