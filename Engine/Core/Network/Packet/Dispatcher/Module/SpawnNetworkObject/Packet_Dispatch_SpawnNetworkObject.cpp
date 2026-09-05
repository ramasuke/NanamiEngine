#include "Packet_Dispatch_SpawnNetworkObject.h"

#include "../../../../../../Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine_Network_INetworkSystem.h"
#include "../../../../../../Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../../../../../../Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "../../../../../../Module/Log/NanamiEngine_Module_Log.h"

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

    NetworkObjectId SpawnNetworkObject::CreateNetworkObjectId()
    {
        const uint32_t pidBits = static_cast<uint8_t>(PlayerId().Value());
        const NetworkObjectId assignedId(pidBits << 16 | nextNetworkObjectId_++ & 0xFFFF);
        return assignedId;
    }

    void SpawnNetworkObject::ApplyNetworkId(
        const NetworkObjectId id,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        instanceRegistry_.RegisterWithId(id, gameObject);
        if (const auto networkGameObject = gameObject->Components().Catch<Module::Network::NetworkGameObject>().lock())
            networkGameObject->SetNetworkObjectId(id);
    }

    NetworkObjectId SpawnNetworkObject::AllocateIdAndRegister(
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto id = CreateNetworkObjectId();
        ApplyNetworkId(id, gameObject);
        return id;
    }

    void SpawnNetworkObject::RegisterWithNetworkId(
        const NetworkObjectId id,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        ApplyNetworkId(id, gameObject);
    }

    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::SpawnAndRegisterWithId(
        const NetworkObjectId assignedId,
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation).lock();
        if (gameObject)
            ApplyNetworkId(assignedId, gameObject);
        return gameObject;
    }

    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::SpawnAndRegister(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        return SpawnAndRegisterWithId(CreateNetworkObjectId(), prefabFile, position, rotation);
    }

    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::DispatchSendPacket(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        // .prefab の読み込みに失敗している場合は Content() が null
        const auto prefabContent = prefabFile.Content();
        if (!prefabContent)
        {
            Module::LogError("SpawnNetworkObject: Prefab の内容が読み込まれていないため Spawn できません: " + prefabFile.GetContentPath());
            return nullptr;
        }

        const auto assignedId = CreateNetworkObjectId();
        Packet packet = Packet::Create(DefaultPacketType::SpawnNetworkObject);
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabContent->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(assignedId);

        const auto gameObject = SpawnAndRegisterWithId(assignedId, prefabFile, position, rotation);
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
