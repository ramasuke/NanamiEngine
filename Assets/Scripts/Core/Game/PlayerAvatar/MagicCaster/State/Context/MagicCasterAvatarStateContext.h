#pragma once
#include <memory>

#include "../../../IPlayerAvatar.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../State/Context/IPlayerAvatarStateContext.h"
#include "../../../CameraGroup/PlayerAvatarCameraGroupBase.h"

namespace NanamiEngine::Module::Asset
{
    class MagicCasterAvatarResource;
}

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarInputAction;
    class MagicCasterAvatarStatus;

    class MagicCasterAvatarStateContext final : public IPlayerAvatarStateContext
    {
    public:
        explicit MagicCasterAvatarStateContext(const std::shared_ptr<MagicCasterAvatarStatus     >& status      ,
                                               const std::shared_ptr<MagicCasterAvatarInputAction>& inputAction ,
                                               const std::weak_ptr<GameObject::IGameObject       >& playerAvatar,
                                               const std::weak_ptr<PlayerAvatarCameraGroupBase   >& cameraGroup ,
                                               const std::weak_ptr<GameObject::IGameObject       >& castPoint   ,
                                               const std::weak_ptr<Asset::MagicCasterAvatarResource>& resources);

        [[nodiscard]] MagicCasterAvatarStatus     & Status() const { return *status_;             }
        [[nodiscard]] MagicCasterAvatarInputAction& Input () const { return *inputAction_;        }
        [[nodiscard]] PlayerAvatarCameraGroupBase & Camera() const { return *cameraGroup_.lock(); }
        [[nodiscard]] bool ExpiredCamera() const { return cameraGroup_.expired(); }

        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> PlayerAvatarObject     () const override { return playerAvatarObject_.lock();                     }
        [[nodiscard]] GameObject::Transform                &   PlayerAvatarTransform  () const override { return playerAvatarObject_.lock()->Transform();     }
        [[nodiscard]] PlayerAvatarCameraGroupBase          &   CameraGroup            () const override { return *cameraGroup_      .lock();                     }
        [[nodiscard]] GamePlay::Ui::NpcChatting            &   NpcChattingUi          () const override { return playerAvatar_      .lock()->NpcChattingUi();    }
        [[nodiscard]] Component::RigidBody                 &   PlayerAvatarRigidBody  () const override { return playerAvatar_      .lock()->RigidBody();        }
        [[nodiscard]] GamePlay::PlayerAvatar::ChattableArea&   ChattableArea          () const override { return playerAvatar_      .lock()->ChattableArea();    }
        [[nodiscard]] GamePlay::PlayerAvatar::WakeUpArea   &   WakeUpArea             () const override { return playerAvatar_      .lock()->WakeUpArea();       }
        [[nodiscard]] const glm::vec3&                         PlayerAvatarFeatStepPos() const override { return playerAvatar_      .lock()->FeatStepPosition(); }
        [[nodiscard]] float                                    GroundCheckRadius      () const override;
        [[nodiscard]] float                                    GroundCheckUpOffset    () const override;
        [[nodiscard]] float                                    GroundCheckDistance    () const override;
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject>   CastPoint              () const { return castPoint_; }
        [[nodiscard]] const Asset::MagicCasterAvatarResource&  Resources              () const { return *resources_.lock(); }

    private:
        const std::shared_ptr<MagicCasterAvatarStatus     > status_;
        const std::weak_ptr  <GameObject::IGameObject     > playerAvatarObject_;
        const std::weak_ptr  <IPlayerAvatar               > playerAvatar_;
        const std::shared_ptr<MagicCasterAvatarInputAction> inputAction_;
        const std::weak_ptr  <PlayerAvatarCameraGroupBase > cameraGroup_;
        const std::weak_ptr  <GameObject::IGameObject     > castPoint_;
        const std::weak_ptr<Asset::MagicCasterAvatarResource> resources_;
    };
}
