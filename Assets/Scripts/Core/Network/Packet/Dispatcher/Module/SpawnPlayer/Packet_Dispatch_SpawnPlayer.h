#pragma once
#define WIN32_LEAN_AND_MEAN
#include <unordered_map>
#include <vector>
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../../../../../../../Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../rxcpp/rx.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::Network
{
    class SpawnPlayerDispatcher final : public CustomDispatcherBase
    {
    public:
        explicit SpawnPlayerDispatcher(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender,
            Asset::PlayerAvatarFactory& playerAvatarFactory);
        ~SpawnPlayerDispatcher() override;

        [[nodiscard]] std::weak_ptr<IPlayerAvatar> DispatchSendPacket(
            PlayerAvatar::PlayerAvatarType type,
            glm::vec3 position,
            glm::quat rotation);

        void OnReceive(const Core::Network::Packet& packet) override;

        Asset::PlayerAvatarFactory& playerAvatarFactory_;

    private:
        // 後入りへ再送するスポーン履歴(ホストのみ保持)。ルートの NetworkObjectId がまだ登録されているものだけ再送する
        // (離脱してアバターが破棄されたプレイヤーの分は自然に除外される)
        struct HistoryEntry
        {
            Core::Network::NetworkObjectId rootId;
            Core::Network::Packet          packet;
        };
        std::vector<HistoryEntry> spawnPacketHistory_;
        rxcpp::composite_subscription newPlayerSubscription_;
        rxcpp::composite_subscription playerLeftSubscription_;
        // キー: PlayerId
        std::unordered_map<int8_t, Asset::PlayerAvatarAttachments> remoteAttachments_;
    };
}
