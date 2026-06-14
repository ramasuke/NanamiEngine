#pragma once
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Game/PlayerAvatar/RequireType/RequireType.h"
#include "../../../../../Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

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
            Core::Network::IPacketSender& packetSender);

        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> DispatchSendPacket(
            Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation) const;

        void OnReceive(const Core::Network::Packet& packet) override;
    };
}
