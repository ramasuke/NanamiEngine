#include "Packet_Dispatch_SpawnPlayer.h"

#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::Network
{
    SpawnPlayerDispatcher::SpawnPlayerDispatcher(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender,
        Asset::PlayerAvatarFactory& playerAvatarFactory,
        PlayerAvatar::AllPlayerCameraGroup cameraGroup)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
            , playerAvatarFactory_(playerAvatarFactory)
            , cameraGroup_(std::move(cameraGroup))
    {
    }

    std::weak_ptr<IPlayerAvatar>
    SpawnPlayerDispatcher::DispatchSendPacket(
        const PlayerAvatar::PlayerAvatarType type,
        const glm::vec3 position,
        const glm::quat rotation) const
    {
        auto playerAvatar = playerAvatarFactory_.LoadInitedPlayerAvatar(type, position, nullptr, cameraGroup_, true);
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        const auto networkObjectId = DefaultDispatch().Spawn().AllocateIdAndRegister(gameObject);

        Core::Network::Packet packet = Core::Network::Packet::Create(
            static_cast<Core::Network::PacketType>(EPacketType::SpawnPlayerAvatar));
        packet.Data().Write(PlayerId());
        packet.Data().Write(static_cast<int>(type));
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(networkObjectId);
        SendPacket(packet);

        return playerAvatar;
    }

    void SpawnPlayerDispatcher::OnReceive(const Core::Network::Packet& packet)
    {
        size_t readOffset = 0;
        const auto playerId        = packet.Data().Read<struct Core::Network::PlayerId>(readOffset);
        const auto avatarTypeInt   = packet.Data().Read<int>(readOffset);
        const auto position        = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation        = packet.Data().Read<glm::quat>(readOffset);
        const auto networkObjectId = packet.Data().Read<Core::Network::NetworkObjectId>(readOffset);

        if (playerId == PlayerId())
            return;

        const auto type = static_cast<GameCore::PlayerAvatar::PlayerAvatarType>(avatarTypeInt);
        auto playerAvatar = playerAvatarFactory_.LoadInitedPlayerAvatar(type, position, nullptr, cameraGroup_, false);
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        DefaultDispatch().Spawn().RegisterWithNetworkId(networkObjectId, gameObject);
    }
}
