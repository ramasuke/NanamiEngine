#include "Packet_Dispatch_SpawnPlayer.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::Network
{
    SpawnPlayerDispatcher::SpawnPlayerDispatcher(
        DefaultPacketDispatcher& defaultDispatchers,
        const IPlayerIdProvider& playerIdProvider,
        IPacketSender& packetSender,
        Asset::PlayerAvatarFactory& playerAvatarFactory)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
            , playerAvatarFactory_(playerAvatarFactory)
    {
        if (IsServer())
        {
            newPlayerSubscription_ = PacketSender().OnConnectPlayer().subscribe(
                [this](const ENetEvent* event)
                {
                    for (const auto& packet : spawnPacketHistory_)
                        PacketSender().SendTo(event->peer, packet);
                },
                [](std::exception_ptr) {}
            );
        }
    }

    SpawnPlayerDispatcher::~SpawnPlayerDispatcher()
    {
        newPlayerSubscription_.unsubscribe();
    }

    std::weak_ptr<IPlayerAvatar>
    SpawnPlayerDispatcher::DispatchSendPacket(
        const PlayerAvatar::PlayerAvatarType type,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        auto playerAvatar = playerAvatarFactory_.LoadInitedPlayerAvatar(type, position, nullptr, true);
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        const auto networkObjectId = DefaultDispatch().Spawn().AllocateIdAndRegister(gameObject);

        Packet packet = Packet::Create(static_cast<PacketType>(EPacketType::SpawnPlayerAvatar));
        packet.Data().Write(PlayerId());
        packet.Data().Write(static_cast<int>(type));
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(networkObjectId);

        if (IsServer())
            spawnPacketHistory_.push_back(packet);
        
        SendPacket(packet);

        return playerAvatar;
    }

    void SpawnPlayerDispatcher::OnReceive(const Packet& packet)
    {
        size_t readOffset = 0;
        const auto playerId        = packet.Data().Read<struct PlayerId>(readOffset);
        const auto avatarTypeInt   = packet.Data().Read<int>(readOffset);
        const auto position        = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation        = packet.Data().Read<glm::quat>(readOffset);
        const auto networkObjectId = packet.Data().Read<NetworkObjectId>(readOffset);

        if (playerId == PlayerId())
            return;

        if (IsServer())
            spawnPacketHistory_.push_back(packet);

        const auto type = static_cast<PlayerAvatar::PlayerAvatarType>(avatarTypeInt);
        auto playerAvatar = playerAvatarFactory_.LoadInitedPlayerAvatar(type, position, nullptr, false);
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        DefaultDispatch().Spawn().RegisterWithNetworkId(networkObjectId, gameObject);
    }
}
