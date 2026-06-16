#pragma once
#include "../glm/vec3.hpp"
#include "../../CustomPacketDispatcherBase.h"
#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Game/PlayerAvatar/CameraGroup/AllPlayerCameraGroup.h"
#include "../../../../../Game/PlayerAvatar/RequireType/RequireType.h"
#include "../../../../../Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../../../../../Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

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
            Asset::PlayerAvatarFactory& playerAvatarFactory,
            PlayerAvatar::AllPlayerCameraGroup cameraGroup);
        
        [[nodiscard]] std::weak_ptr<IPlayerAvatar> DispatchSendPacket(
            PlayerAvatar::PlayerAvatarType type,
            glm::vec3 position,
            glm::quat rotation) const;
        
        void OnReceive(const Core::Network::Packet& packet) override;
        
        Asset::PlayerAvatarFactory& playerAvatarFactory_;
        PlayerAvatar::AllPlayerCameraGroup cameraGroup_;
    };
}
