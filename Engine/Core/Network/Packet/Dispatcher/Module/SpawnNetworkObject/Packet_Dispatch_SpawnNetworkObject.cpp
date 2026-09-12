#include "Packet_Dispatch_SpawnNetworkObject.h"

#include <algorithm>
#include "cereal/types/vector.hpp"
#include "../../../../../../Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine_Network_INetworkSystem.h"
#include "../../../../../../Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../Object/Awakable/INetworkAwakable.h"
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

    bool SpawnNetworkObject::IsNetworkNode(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        if (gameObject->Components().Catch<Module::Network::NetworkGameObject>().lock())
            return true;

        return !gameObject->Components().Catches<INetworkAwakable>().empty();
    }

    std::vector<std::shared_ptr<GameObject::IGameObject>> SpawnNetworkObject::CollectNetworkGameObjects(
        const std::shared_ptr<GameObject::IGameObject>& root) const
    {
        std::vector<std::shared_ptr<GameObject::IGameObject>> nodes;

        if (IsNetworkNode(root))
            nodes.push_back(root);

        for (const auto& child : root->Transform().GetAllChildren())
            if (IsNetworkNode(child))
                nodes.push_back(child);

        return nodes;
    }

    void SpawnNetworkObject::ApplyNetworkIds(
        const std::vector<NetworkObjectId>& ids,
        const std::shared_ptr<GameObject::IGameObject>& gameObject,
        const OwnerLeavePolicy policy)
    {
        const auto nodes = CollectNetworkGameObjects(gameObject);
        if (nodes.size() != ids.size())
            Module::LogError("SpawnNetworkObject: NetworkGameObject の数(" + std::to_string(nodes.size()) +
                ")と受け取った NetworkObjectId の数(" + std::to_string(ids.size()) + ")が一致していません");

        const auto count = std::min(nodes.size(), ids.size());
        for (size_t i = 0; i < count; ++i)
        {
            instanceRegistry_.RegisterWithId(ids[i], nodes[i], policy);

            // NetworkGameObject があればそれ経由で同一 GameObject 上の NetworkComponent へ配る。
            // 無い(NetworkComponent だけを持つ子オブジェクト)場合は直接 NetworkAwake で配る
            if (const auto networkGameObject = nodes[i]->Components().Catch<Module::Network::NetworkGameObject>().lock())
            {
                networkGameObject->SetNetworkObjectId(ids[i]);
                continue;
            }
            for (const auto& awakable : nodes[i]->Components().Catches<INetworkAwakable>())
                awakable.lock()->NetworkAwake(ids[i]);
        }
    }

    std::vector<NetworkObjectId> SpawnNetworkObject::AllocateIdsAndRegister(
        const std::shared_ptr<GameObject::IGameObject>& gameObject,
        const OwnerLeavePolicy policy)
    {
        const auto nodeCount = CollectNetworkGameObjects(gameObject).size();

        std::vector<NetworkObjectId> ids;
        ids.reserve(nodeCount);
        for (size_t i = 0; i < nodeCount; ++i)
            ids.push_back(CreateNetworkObjectId());

        ApplyNetworkIds(ids, gameObject, policy);
        return ids;
    }

    void SpawnNetworkObject::RegisterWithNetworkIds(
        const std::vector<NetworkObjectId>& ids,
        const std::shared_ptr<GameObject::IGameObject>& gameObject,
        const OwnerLeavePolicy policy)
    {
        ApplyNetworkIds(ids, gameObject, policy);
    }

    void SpawnNetworkObject::DespawnAndUnregister(const std::shared_ptr<GameObject::IGameObject>& root)
    {
        if (!root)
            return;

        for (const auto& node : CollectNetworkGameObjects(root))
            instanceRegistry_.UnregisterObject(node);

        // 破棄は GameWindow の削除キューに積まれ、子オブジェクトも一緒に破棄される
        root->OnDestroy();
    }

    std::shared_ptr<GameObject::IGameObject> SpawnNetworkObject::SpawnAndRegister(
        Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation).lock();
        if (gameObject)
            AllocateIdsAndRegister(gameObject, OwnerLeavePolicy::Transfer);
        return gameObject;
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

        const auto gameObject = Scene::GameObject::Instantiate(prefabFile, position, rotation).lock();
        if (!gameObject)
            return nullptr;

        const auto assignedIds = AllocateIdsAndRegister(gameObject, OwnerLeavePolicy::Transfer);

        Packet packet = Packet::Create(DefaultPacketType::SpawnNetworkObject);
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabContent->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(assignedIds);

        SendPacket(packet);
        return gameObject;
    }

    void SpawnNetworkObject::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const auto playerId         = packet.Data().Read<struct PlayerId>(offset);
        const auto spawnObjectGuid  = packet.Data().Read<Guid>(offset);
        const auto position         = packet.Data().Read<glm::vec3>(offset);
        const auto rotation         = packet.Data().Read<glm::quat>(offset);
        const auto networkObjectIds = packet.Data().Read<std::vector<NetworkObjectId>>(offset);

        if (playerId == PlayerId())
            return;

        const auto spawnObject = NetworkObjectRegistry().Catch(spawnObjectGuid);
        if (spawnObject.expired())
            return;

        const auto gameObject = Scene::GameObject::Instantiate(*spawnObject.lock(), position, rotation).lock();
        if (gameObject)
            ApplyNetworkIds(networkObjectIds, gameObject, OwnerLeavePolicy::Transfer);
    }
}
