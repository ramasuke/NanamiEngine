#include "CustomPacketDispatcherGroup.h"

#include "../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../Custom_PacketType.h"

namespace GameCore::Network
{
    CustomDispatcherGroup::CustomDispatcherGroup(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        Core::Network::IPacketSender& packetSender,
        Core::Network::IPlayerIdProvider& playerIdProvider)
        // Note: SpawnPlayerDispatcher のコンストラクタ引数順は (DefaultDispatcher, PlayerIdProvider, PacketSender)
        //       CustomDispatcherGroup の引数順とは異なる点に注意
        : spawnPlayerDispatcher_(defaultDispatchers, playerIdProvider, packetSender)
    {
    }

    void CustomDispatcherGroup::DispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        const auto customType = static_cast<EPacketType>(packet.Type());
        switch (customType)
        {
        case EPacketType::SpawnPlayerAvatar:
            // ReceivePacket() を呼ぶことで基底クラスが Relay/Authoritative/Client を自動振り分け
            spawnPlayerDispatcher_.ReceivePacket(packet);
            break;
        }
    }
}
