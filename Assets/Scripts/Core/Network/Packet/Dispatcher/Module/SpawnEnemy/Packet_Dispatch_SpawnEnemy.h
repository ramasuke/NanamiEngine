#pragma once
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../../../../Data/Enemy/Factory/EnemyFactory.h"
#include "../../../../../Game/Npc/Enemy/Type/EnemyKind.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "Packages/R4/R4.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Network
{
    class EnemySpawnDispatcher final : public CustomDispatcherBase
    {
    public:
        explicit EnemySpawnDispatcher(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender,
            Asset::EnemyFactory& enemyFactory);
        ~EnemySpawnDispatcher() override;

        std::shared_ptr<Module::GameObject::IGameObject> DispatchSendPacket(
            Npc::Enemy::EnemyKind kind,
            glm::vec3 position,
            glm::quat rotation);

        void OnReceive(const Core::Network::Packet& packet) override;

    private:
        // 後入りへ再送するスポーン履歴(ホストのみ保持)。ルートの NetworkObjectId がまだ登録されているものだけ再送する
        struct HistoryEntry
        {
            Core::Network::NetworkObjectId rootId;
            Core::Network::Packet          packet;
        };
        std::vector<HistoryEntry> spawnPacketHistory_;
        R4::Disposable newPlayerSubscription_;
        Asset::EnemyFactory& enemyFactory_;
    };
}
