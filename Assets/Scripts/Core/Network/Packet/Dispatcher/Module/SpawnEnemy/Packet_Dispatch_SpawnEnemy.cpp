#include "Packet_Dispatch_SpawnEnemy.h"

#include "cereal/types/vector.hpp"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../Engine/Core/Network/Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "../../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"

namespace GameCore::Network
{
    EnemySpawnDispatcher::EnemySpawnDispatcher(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
    {
        if (IsServer())
        {
            newPlayerSubscription_ = PacketSender().OnConnectPlayer().subscribe(
                [this](const ENetEvent* event)
                {
                    // 既に破棄された敵の履歴は再送せずに捨てる
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
    }

    EnemySpawnDispatcher::~EnemySpawnDispatcher()
    {
        newPlayerSubscription_.unsubscribe();
    }

    std::shared_ptr<Module::GameObject::IGameObject>
    EnemySpawnDispatcher::DispatchSendPacket(
        Module::Asset::PrefabGameObjectFile& prefab,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        // .prefab の読み込みに失敗している場合は Content() が null
        const auto prefabContent = prefab.Content();
        if (!prefabContent)
        {
            Module::LogError("EnemySpawnDispatcher: Prefab の内容が読み込まれていないため Spawn できません: " + prefab.GetContentPath());
            return nullptr;
        }

        const auto gameObject = Scene::GameObject::Instantiate(prefab, position, rotation).lock();
        if (!gameObject)
            return nullptr;

        // 敵は Spawn したプレイヤーが離脱しても残し、所有権をホストへ移す
        const auto networkObjectIds = DefaultDispatch().Spawn().AllocateIdsAndRegister(gameObject, Core::Network::OwnerLeavePolicy::Transfer);

        Core::Network::Packet packet = Core::Network::Packet::Create(static_cast<Core::Network::PacketType>(EPacketType::SpawnEnemy));
        packet.Data().Write(PlayerId());
        packet.Data().Write(prefabContent->GetGuid());
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.Data().Write(networkObjectIds);

        if (IsServer() && !networkObjectIds.empty())
            spawnPacketHistory_.push_back({ networkObjectIds.front(), packet });

        SendPacket(packet);

        return gameObject;
    }

    void EnemySpawnDispatcher::OnReceive(const Core::Network::Packet& packet)
    {
        size_t readOffset = 0;
        const auto playerId         = packet.Data().Read<Core::Network::PlayerId>(readOffset);
        const auto prefabGuid       = packet.Data().Read<Guid>(readOffset);
        const auto position         = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation         = packet.Data().Read<glm::quat>(readOffset);
        const auto networkObjectIds = packet.Data().Read<std::vector<Core::Network::NetworkObjectId>>(readOffset);

        if (playerId == PlayerId())
            return;

        if (IsServer() && !networkObjectIds.empty())
            spawnPacketHistory_.push_back({ networkObjectIds.front(), packet });

        const auto spawnObject = NetworkObjectRegistry().Catch(prefabGuid);
        if (spawnObject.expired())
            return;

        const auto gameObject = Scene::GameObject::Instantiate(*spawnObject.lock(), position, rotation).lock();
        if (!gameObject)
            return;

        DefaultDispatch().Spawn().RegisterWithNetworkIds(networkObjectIds, gameObject, Core::Network::OwnerLeavePolicy::Transfer);
    }
}
