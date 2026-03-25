#pragma once
#include "../../IPlayerAvatar.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
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
        [[nodiscard]] virtual Physics::ICollider                   &   PlayerAvatarCollider     () const = 0;
        [[nodiscard]] virtual GameObject::Transform                &   PlayerAvatarTransform    () const = 0;
        [[nodiscard]] virtual GamePlay::Ui::NpcChatting            &   NpcChattingUi            () const = 0;
        [[nodiscard]] virtual PlayerAvatarCameraGroupBase          &   CameraGroup              () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::ChattableArea&   ChattableArea            () const = 0;
        [[nodiscard]] virtual const glm::vec3&                         PlayerAvatarFeatStepPos  () const = 0;
        
    };
}
