#include "Packet_Dispatch_SpawnNetworkObject.h"

#include "../../../../../../Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine_Network_INetworkSystem.h"
#include "../../../../../../Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../../../../../../Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"

namespace NanamiEngine::Core::Network
{
    SpawnNetworkObject::SpawnNetworkObject(
        const IPlayerIdProvider& playerIdProvider,
        IPacketSender& packetSender,
        INetworkObjectInstanceRegistry& instanceRegistry)
        : PacketDispatcherBase(playerIdProvider, packetSender)
        , instanceRegistry_(instanceRegistry)
    {
    }

    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::DispatchSendPacket(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        
        const uint32_t pidBits = static_cast<uint32_t>(static_cast<uint8_t>(PlayerId().Value()));
        const NetworkObjectId assignedId(pidBits << 16 | nextNetworkObjectId_++ & 0xFFFF);

        Packet packet = Packet::Create(DefaultPacketType::SpawnNetworkObject);
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabFile.Content()->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(assignedId);

        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation).lock();
        if (gameObject)
        {
            instanceRegistry_.RegisterWithId(assignedId, gameObject);
            if (const auto ngObj = gameObject->Components().Catch<Module::Network::NetworkGameObject>().lock())
                ngObj->SetNetworkObjectId(assignedId);
        }

        SendPacket(packet);
        return gameObject;
    }

    void SpawnNetworkObject::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const auto playerId        = packet.Data().Read<struct PlayerId>(offset);
        const auto spawnObjectGuid = packet.Data().Read<Guid>(offset);
        const auto position        = packet.Data().Read<glm::vec3>(offset);
        const auto rotation        = packet.Data().Read<glm::quat>(offset);
        const auto networkObjectId = packet.Data().Read<NetworkObjectId>(offset);

        if (playerId == PlayerId())
            return;

        const auto spawnObject = NetworkObjectRegistry().Catch(spawnObjectGuid);
        if (spawnObject.expired())
            return;

        const auto gameObject = Scene::GameObject::Instantiate(*spawnObject.lock(), position, rotation).lock();
        if (gameObject)
        {
            instanceRegistry_.RegisterWithId(networkObjectId, gameObject);
            if (const auto networkInstaceObject = gameObject->Components().Catch<Module::Network::NetworkGameObject>().lock())
                networkInstaceObject->SetNetworkObjectId(networkObjectId);
        }
    }
}
