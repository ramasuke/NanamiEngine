#pragma once
#include "../../IPlayerAvatar.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Component
{
    class RigidBody;
}

namespace GamePlay::Ui
{
    class NpcChatting;
}

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarCameraGroupBase;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStateContext
    {
    public:
        virtual ~IPlayerAvatarStateContext() = default;

        [[nodiscard]] virtual std::shared_ptr<GameObject::IGameObject> PlayerAvatarObject       () const = 0;
        [[nodiscard]] virtual Component::RigidBody                 &   PlayerAvatarRigidBody    () const = 0;
        [[nodiscard]] virtual GameObject::Transform                &   PlayerAvatarTransform    () const = 0;
        [[nodiscard]] virtual GamePlay::Ui::NpcChatting            &   NpcChattingUi            () const = 0;
        [[nodiscard]] virtual PlayerAvatarCameraGroupBase          &   CameraGroup              () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::ChattableArea&   ChattableArea            () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::WakeUpArea   &   WakeUpArea               () const = 0;
        [[nodiscard]] virtual const glm::vec3&                         PlayerAvatarFeatStepPos  () const = 0;
        [[nodiscard]] virtual float                                    GroundCheckRadius        () const = 0;
        [[nodiscard]] virtual float                                    GroundCheckUpOffset      () const = 0;
        [[nodiscard]] virtual float                                    GroundCheckDistance      () const = 0;

    };
}
