#include "Packet_Dispatch_SpawnPlayer.h"
#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../Engine/Core/Network/Object/Registry/NetworkObjectRegistry.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::Network
{
    SpawnPlayerDispatcher::SpawnPlayerDispatcher(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
    {
    }

    std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>
    SpawnPlayerDispatcher::DispatchSendPacket(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation) const
    {
        Core::Network::Packet packet = Core::Network::Packet::Create(
            static_cast<Core::Network::PacketType>(EPacketType::SpawnPlayerAvatar));
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabFile.Content()->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);

        // SummonSwordManAvatar パターン: Instantiate してコンポーネントを取得
        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation).lock();
        auto swordManAvatar = gameObject->Components().Catch<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>();

        SendPacket(packet);

        return swordManAvatar;
    }

    void SpawnPlayerDispatcher::OnReceive(const Core::Network::Packet& packet)
    {
        size_t readOffset = 0;
        const auto playerId        = packet.Data().Read<struct Core::Network::PlayerId>(readOffset);
        const auto spawnObjectGuid = packet.Data().Read<Guid>(readOffset);
        const auto position        = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation        = packet.Data().Read<glm::quat>(readOffset);

        if (playerId == PlayerId())
            return;

        const auto spawnObject = NetworkObjectRegistry().Catch(spawnObjectGuid);
        if (spawnObject.expired())
            return;

        Scene::GameObject::Instantiate(*spawnObject.lock(), position, rotation);
    }
}
