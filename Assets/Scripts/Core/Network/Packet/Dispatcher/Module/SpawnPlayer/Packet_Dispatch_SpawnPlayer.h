#pragma once
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../rxcpp/rx.hpp"

namespace NanamiEngine::Module::Asset
{
    class PlayerAvatarFactory;
}

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
        std::vector<Core::Network::Packet> spawnPacketHistory_;
        rxcpp::composite_subscription newPlayerSubscription_;
    };
}
