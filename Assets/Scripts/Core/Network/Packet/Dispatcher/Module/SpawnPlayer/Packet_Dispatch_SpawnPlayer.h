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

namespace GameCore::Network
{
    class SpawnPlayerDispatcher final : public CustomDispatcherBase
    {
    public:
        explicit SpawnPlayerDispatcher(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender);

        // template <typename AvatarT, typename TraitsT>
        // std::shared_ptr<IPlayerAvatar> DispatchSendPacket(
        //     const std::shared_ptr<Asset::PrefabGameObjectFile>& prefabFile,
        //     const glm::vec3 summonPosition,
        //     const glm::quat rotation,
        //     std::shared_ptr<PlayerAvatar::RequireType::CameraGroup<TraitsT>> cameraGroup)
        // {
        //     using namespace GameCore::PlayerAvatar;
        //     using Status = RequireType::Status<TraitsT>;
        //     using Input  = RequireType::InputAction<TraitsT>;
        //
        //     //Create
        //     Core::Network::Packet packet = Core::Network::Packet::Create(EPacketType::SpawnPlayerAvatar);
        //     packet.Data().Write(prefabFile->Content()->GetGuid());
        //     packet.Data().Write(summonPosition);
        //     packet.Data().Write(rotation);
        //     packet.Data().Write(LoadStatus<Status, TraitsT>());
        //     
        //     auto playerAvatarObject = Scene::GameObject::Instantiate(prefabFile, summonPosition).lock();
        //     auto playerAvatar = playerAvatarObject->Components().Catch<AvatarT>().lock();
        //
        //     //Init
        //     auto inputAction  = std::make_shared<Input>();
        //     auto status = LoadStatus<Status, TraitsT>();
        //     auto stateMachine = TraitsT::CreateStateMachine(status, inputAction, playerAvatar, cameraGroup);
        //
        //     playerAvatar->Init(
        //         status,
        //         std::move(stateMachine),
        //         inputAction,
        //         cameraGroup);
        //
        //     playerAvatarObject->Transform().SetWorldPos(summonPosition);
        //     playerAvatarObject->Transform().SetWorldRot(summonPosition);
        //     return playerAvatar;
        // }
    };
}
