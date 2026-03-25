#pragma once
#include <memory>

#include "../../../IPlayerAvatar.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../AttackArea/PlayerAvatarAttackArea.h"
#include "../../../State/Context/IPlayerAvatarStateContext.h"
#include "../../CameraGroup/SwordManAvatarCameraGroup.h"

namespace NanamiEngine::Module::Asset
{
    class SoundFile;
}

namespace NanamiEngine::Module::Component
{
    class ParticleSystem;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarInputAction;
    class SwordManAvatarStatus;

    class SwordManAvatarStateContext final : public IPlayerAvatarStateContext
    {
    public:
        explicit SwordManAvatarStateContext(const std::shared_ptr<SwordManAvatarStatus     >& status      ,
                                            const std::shared_ptr<SwordManAvatarInputAction>& inputAction ,
                                            const std::weak_ptr  <GameObject::IGameObject  >& playerAvatar,
                                            const std::weak_ptr  <SwordManAvatarCameraGroup>& cameraGroup ,
                                            const std::weak_ptr  <PlayerAttackArea>& normalAttackArea,
                                            const std::weak_ptr  <PlayerAttackArea>& dashAttackArea,
                                            const std::weak_ptr<Component::ParticleSystem>& onReinforceParticle,
                                            const std::weak_ptr<Component::ParticleSystem>& reinforcingParticle,
                                            const std::weak_ptr<Asset::SoundFile         >& normalAttackSound,
                                            const std::weak_ptr<Asset::SoundFile         >& avoidRollingSound);
        
        [[nodiscard]] SwordManAvatarStatus     & Status () const { return *status_;             }
        [[nodiscard]] SwordManAvatarInputAction& Input  () const { return *inputAction_;        }
        [[nodiscard]] SwordManAvatarCameraGroup& Camera () const { return *cameraGroup_.lock(); }

        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> PlayerAvatarObject     () const override { return playerAvatarObject_.lock();                     }
        [[nodiscard]] GameObject::Transform                &   PlayerAvatarTransform  () const override { return playerAvatarObject_.lock()->TransformRef();     }
        [[nodiscard]] PlayerAvatarCameraGroupBase          &   CameraGroup            () const override { return *cameraGroup_      .lock();                     }
        [[nodiscard]] GamePlay::Ui::NpcChatting            &   NpcChattingUi          () const override { return playerAvatar_      .lock()->NpcChattingUi();    }
        [[nodiscard]] Physics::ICollider                   &   PlayerAvatarCollider   () const override { return playerAvatar_      .lock()->Collider();         }
        [[nodiscard]] GamePlay::PlayerAvatar::ChattableArea&   ChattableArea          () const override { return playerAvatar_      .lock()->ChattableArea();    }
        [[nodiscard]] const glm::vec3&                         PlayerAvatarFeatStepPos() const override { return playerAvatar_      .lock()->FeatStepPosition(); }
        [[nodiscard]] PlayerAttackArea& NormalAttackArea() const { return *normalAttackArea_.lock(); }
        [[nodiscard]] PlayerAttackArea& DashAttackArea  () const { return *dashAttackArea_  .lock(); }
        [[nodiscard]] Component::ParticleSystem& OnReinforceParticle() const { return *onReinforceParticle_.lock(); } 
        [[nodiscard]] Component::ParticleSystem& ReinforcingParticle() const { return *reinforcingParticle_.lock(); }
        [[nodiscard]] const Asset::SoundFile   & NormalAttackSound  () const { return *normalAttackSound_  .lock(); }
        [[nodiscard]] const Asset::SoundFile   & AvoidRollingSound  () const { return *avoidRollingSound_  .lock(); }


    private:
        const std::shared_ptr<SwordManAvatarStatus     > status_;
        const std::weak_ptr  <GameObject::IGameObject  > playerAvatarObject_;
        const std::weak_ptr  <IPlayerAvatar            > playerAvatar_;
        const std::shared_ptr<SwordManAvatarInputAction> inputAction_;
        const std::weak_ptr  <SwordManAvatarCameraGroup> cameraGroup_; 
        const std::weak_ptr  <PlayerAttackArea> normalAttackArea_; 
        const std::weak_ptr  <PlayerAttackArea> dashAttackArea_;
        const std::weak_ptr<Component::ParticleSystem> onReinforceParticle_;
        const std::weak_ptr<Component::ParticleSystem> reinforcingParticle_;
        const std::weak_ptr<Asset::SoundFile> normalAttackSound_;
        const std::weak_ptr<Asset::SoundFile> avoidRollingSound_;
        
    };
}
