#include "Packet_Dispatch_SpawnPlayer.h"

#include "cereal/types/vector.hpp"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../Game/PlayerAvatar/Status/NullPlayerAvatarStatus.h"

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
                    // 既に破棄されたアバター(離脱者)の履歴は再送せずに捨てる
                    for (auto it = spawnPacketHistory_.begin(); it != spawnPacketHistory_.end();)
                    {
                        if (DefaultDispatch().FindNetworkObject(it->rootId).lock())
                        {
                            PacketSender().SendTo(event->peer, it->packet);
                            ++it;
                        }
                        else
                        {
                            it = spawnPacketHistory_.erase(it);
                        }
                    }
                },
                [](std::exception_ptr) {}
            );
        }

        // 離脱者のアバター本体は SessionDispatcher が破棄済み。ここでは付属のステータスUI等を片付ける
        playerLeftSubscription_ = DefaultDispatch().Session().OnPlayerLeft().subscribe(
            [this](const Core::Network::PlayerId left)
            {
                const auto it = remoteAttachments_.find(left.Value());
                if (it == remoteAttachments_.end())
                    return;
                playerAvatarFactory_.DestroyAttachments(it->second);
                remoteAttachments_.erase(it);
            },
            [](std::exception_ptr) {}
        );
    }

    SpawnPlayerDispatcher::~SpawnPlayerDispatcher()
    {
        newPlayerSubscription_.unsubscribe();
        playerLeftSubscription_.unsubscribe();
    }

    std::weak_ptr<IPlayerAvatar>
    SpawnPlayerDispatcher::DispatchSendPacket(
        const PlayerAvatar::PlayerAvatarType type,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        auto playerAvatar = playerAvatarFactory_.LoadInitedPlayerAvatar(
            type, position, nullptr, true, std::make_shared<PlayerAvatar::NullPlayerAvatarStatus>());
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        const auto networkObjectIds = DefaultDispatch().Spawn().AllocateIdsAndRegister(gameObject, Core::Network::OwnerLeavePolicy::Destroy, PlayerId());

        Packet packet = Packet::Create(static_cast<PacketType>(EPacketType::SpawnPlayerAvatar));
        packet.Data().Write(PlayerId()); // 送信者 兼 初期所有者
        packet.Data().Write(static_cast<int>(type));
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(networkObjectIds);

        // PlayerStatus()は参照しか返さないため、cerealのポリモーフィックシリアライズに渡すために
        // 所有権を持たないshared_ptrでラップする(deleterは何もしない)。
        GameCore::PlayerAvatar::IPlayerAvatarStatus* statusPtr = &playerAvatar->PlayerStatus();
        auto statusNoopDeleter = [](GameCore::PlayerAvatar::IPlayerAvatarStatus*) {};
        const std::shared_ptr<GameCore::PlayerAvatar::IPlayerAvatarStatus> status(statusPtr, statusNoopDeleter);
        packet.Data().Write(status);

        if (IsServer() && !networkObjectIds.empty())
            spawnPacketHistory_.push_back({ networkObjectIds.front(), packet });
        
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
        const auto networkObjectIds = packet.Data().Read<std::vector<NetworkObjectId>>(readOffset);
        const auto status           = packet.Data().Read<std::shared_ptr<GameCore::PlayerAvatar::IPlayerAvatarStatus>>(readOffset);

        if (playerId == PlayerId())
            return;

        if (IsServer() && !networkObjectIds.empty())
            spawnPacketHistory_.push_back({ networkObjectIds.front(), packet });

        const auto type = static_cast<PlayerAvatar::PlayerAvatarType>(avatarTypeInt);
        auto loaded = playerAvatarFactory_.LoadInitedPlayerAvatarWithAttachments(type, position, nullptr, false, status);
        auto playerAvatar = loaded.avatar;
        remoteAttachments_[playerId.Value()] = loaded.attachments;
        auto gameObject = playerAvatar->PlayerTransform().GetGameObject();

        gameObject->Transform().SetWorldRot(rotation);
        DefaultDispatch().Spawn().RegisterWithNetworkIds(networkObjectIds, gameObject, Core::Network::OwnerLeavePolicy::Destroy, playerId);
    }
}
