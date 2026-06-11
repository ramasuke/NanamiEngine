#include "Packet_Dispatch_SpawnNetworkObject.h"

#include "../../../../../../Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine_Network_INetworkSystem.h"
#include "../../../../../../Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Object/Registry/NetworkObjectRegistry.h"

namespace NanamiEngine::Core::Network
{
    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::DispatchSendPacket(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation) const
    {
        Packet packet = Packet::Create(DefaultPacketType::SpawnNetworkObject);
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabFile.Content()->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);

        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation);
        SendPacket(packet);
        return gameObject.lock();
    }

    void SpawnNetworkObject::ReceivePacket(
        const Packet& packet)
    {
        size_t readOffset = 0;
        const auto playerId        = packet.Data().Read<struct PlayerId>(readOffset);
        const auto spawnObjectGuid = packet.Data().Read<Guid>(readOffset);
        const auto position        = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation        = packet.Data().Read<glm::quat>(readOffset);

        if (playerId == PlayerId())
            return;
        
        const auto spawnObject = NetworkObjectRegistry().Catch(spawnObjectGuid);
        Scene::GameObject::Instantiate(*spawnObject.lock(), position, rotation);
    }
}
