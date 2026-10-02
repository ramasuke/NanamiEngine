#include "Packet_Dispatch_SpawnEnemy.h"

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"

namespace GameCore::Network
{
    namespace
    {
        // NOTE: 見つからなくても kind の prefab で湧かせる。湧かせないと NetworkObjectId がホストとずれる
        std::shared_ptr<Asset::PrefabGameObjectFile> ResolvePrefab(const std::string& prefabGuid)
        {
            if (prefabGuid.empty())
                return nullptr;

            const auto prefab = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                .Catch<Asset::PrefabGameObjectFile>(Guid(prefabGuid)).lock();
            if (!prefab)
                NanamiEngine::Module::LogWarning("SpawnEnemy: PrefabGameObjectFile が見つかりません (guid:" + prefabGuid + ")");
            return prefab;
        }
    }

    EnemySpawnDispatcher::EnemySpawnDispatcher(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender,
        Asset::EnemyFactory& enemyFactory)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
            , enemyFactory_(enemyFactory)
    {
        // NOTE: 中継サーバー経由ではホストかどうかが接続後に決まるので、ここでは IsServer() で絞らない(通知はホストにしか来ない)
        newPlayerSubscription_ = PacketSender().OnConnectPlayer().Subscribe(
            [this](const Core::Network::PlayerId joined)
            {
                // 既に破棄された敵の履歴は再送せずに捨てる
                for (auto it = spawnPacketHistory_.begin(); it != spawnPacketHistory_.end();)
                {
                    if (DefaultDispatch().FindNetworkObject(it->rootId).lock())
                    {
                        PacketSender().SendTo(joined, it->packet);
                        ++it;
                    }
                    else
                    {
                        it = spawnPacketHistory_.erase(it);
                    }
                }
            });
    }

    EnemySpawnDispatcher::~EnemySpawnDispatcher()
    {
        newPlayerSubscription_.Dispose();
    }

    std::shared_ptr<Module::GameObject::IGameObject>
    EnemySpawnDispatcher::DispatchSendPacket(
        const Npc::Enemy::EnemyKind kind,
        const std::shared_ptr<Asset::PrefabGameObjectFile>& prefab,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        // プレハブの解決と生成後の配線は EnemyFactory に任せる。受信側も同じ Summon を通る
        const auto gameObject = enemyFactory_.Summon(kind, prefab, position, rotation).lock();
        if (!gameObject)
            return nullptr;

        // 敵は Spawn したプレイヤーが離脱しても残し、所有権をホストへ移す
        const auto networkObjectIds = DefaultDispatch().Spawn().AllocateIdsAndRegister(gameObject, Core::Network::OwnerLeavePolicy::Transfer, PlayerId());

        Core::Network::Packet packet = Core::Network::Packet::Create(static_cast<Core::Network::PacketType>(EPacketType::SpawnEnemy));
        packet.Data().Write(PlayerId()); // 送信者 兼 初期所有者
        packet.Data().Write(kind);
        // NOTE: 湧き地点で prefab を指定したときだけ guid が入る。空なら受信側も kind の prefab
        packet.Data().Write(prefab ? prefab->GetGuid().Value() : std::string());
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
        const auto kind             = packet.Data().Read<Npc::Enemy::EnemyKind>(readOffset);
        const auto prefabGuid       = packet.Data().Read<std::string>(readOffset);
        const auto position         = packet.Data().Read<glm::vec3>(readOffset);
        const auto rotation         = packet.Data().Read<glm::quat>(readOffset);
        const auto networkObjectIds = packet.Data().Read<std::vector<Core::Network::NetworkObjectId>>(readOffset);

        if (playerId == PlayerId())
            return;

        if (IsServer() && !networkObjectIds.empty())
            spawnPacketHistory_.push_back({ networkObjectIds.front(), packet });

        const auto gameObject = enemyFactory_.Summon(kind, ResolvePrefab(prefabGuid), position, rotation).lock();
        if (!gameObject)
            return;

        DefaultDispatch().Spawn().RegisterWithNetworkIds(networkObjectIds, gameObject, Core::Network::OwnerLeavePolicy::Transfer, playerId);
    }
}
