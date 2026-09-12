#pragma once
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../rxcpp/rx.hpp"

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

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
            Core::Network::IPacketSender& packetSender);
        ~EnemySpawnDispatcher() override;

        std::shared_ptr<Module::GameObject::IGameObject> DispatchSendPacket(
            Module::Asset::PrefabGameObjectFile& prefab,
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
        rxcpp::composite_subscription newPlayerSubscription_;
    };
}
