#pragma once
#include <memory>

#include "../../../IPlayerAvatar.h"
#include "Engine/Module/Namespace/EngineNamespace.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../State/Context/IPlayerAvatarStateContext.h"
#include "../../../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../../../../Magic/IMagicCaster.h"
#include "../../../../Magic/IMagicSpell.h"
#include "../../../../../../GamePlay/PlayerAvatar/LockOnDetectionArea/LockOnDetectionArea.h"

namespace NanamiEngine::Module::Asset
{
    class MagicCasterAvatarResource;
}

namespace GamePlay::PlayerAvatar
{
    class LockOnDetectionArea;
}

namespace GameCore::Magic
{
    class IMagicCaster;
    class IMagicSpell;
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
                                               const std::weak_ptr<Asset::MagicCasterAvatarResource>& resources ,
                                               const std::weak_ptr<Magic::IMagicCaster           >& caster      ,
                                               const std::weak_ptr<GamePlay::PlayerAvatar::LockOnDetectionArea>& lockOnDetectionArea);

        [[nodiscard]] MagicCasterAvatarStatus     & Status() const { return *status_;             }
        [[nodiscard]] MagicCasterAvatarInputAction& Input () const { return *inputAction_;        }
        [[nodiscard]] PlayerAvatarCameraGroupBase & Camera() const { return *cameraGroup_.lock(); }
        [[nodiscard]] bool ExpiredCamera() const { return cameraGroup_.expired(); }

        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> PlayerAvatarObject     () const override { return playerAvatarObject_.lock();                     }
        [[nodiscard]] GameObject::Transform                &   PlayerAvatarTransform  () const override { return playerAvatarObject_.lock()->Transform();     }
        [[nodiscard]] PlayerAvatarCameraGroupBase          &   CameraGroup            () const override { return *cameraGroup_      .lock();                     }
        [[nodiscard]] GamePlay::Ui::NpcChatting            &   NpcChattingUi          () const override { return playerAvatar_      .lock()->NpcChattingUi();    }
        [[nodiscard]] Component::RigidBody                 &   PlayerAvatarRigidBody  () const override { return playerAvatar_      .lock()->RigidBody();        }
        [[nodiscard]] GamePlay::PlayerAvatar::InteractableArea&   InteractableArea          () const override { return playerAvatar_      .lock()->InteractableArea();    }
        [[nodiscard]] GamePlay::PlayerAvatar::WakeUpArea   &   WakeUpArea             () const override { return playerAvatar_      .lock()->WakeUpArea();       }
        [[nodiscard]] const glm::vec3&                         PlayerAvatarFeatStepPos() const override { return playerAvatar_      .lock()->FeatStepPosition(); }
        [[nodiscard]] float                                    GroundCheckRadius      () const override;
        [[nodiscard]] float                                    GroundCheckUpOffset    () const override;
        [[nodiscard]] float                                    GroundCheckDistance    () const override;
        [[nodiscard]] float                                    MaxWalkableSlope_deg   () const override;
        [[nodiscard]] float                                    SlopeCheckRadius       () const override;
        [[nodiscard]] float                                    SlopeCheckUpOffset     () const override;
        [[nodiscard]] float                                    SlopeCheckDistance     () const override;
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject>   CastPoint              () const { return castPoint_; }
        [[nodiscard]] const Asset::MagicCasterAvatarResource&  Resources              () const { return *resources_.lock(); }
        [[nodiscard]] Magic::IMagicCaster&                     Caster                 () const { return *caster_.lock(); }
        [[nodiscard]] bool ExpiredLockOnDetectionArea() const { return lockOnDetectionArea_.expired(); }
        [[nodiscard]] GamePlay::PlayerAvatar::LockOnDetectionArea& LockOnDetectionArea() const { return *lockOnDetectionArea_.lock(); }

        /** @brief Cast State に入る直前に、撃つ枠と魔法を置く */
        void SetPendingCast(int slot, const std::shared_ptr<const Magic::IMagicSpell>& spell) { pendingSpellSlot_ = slot; pendingSpell_ = spell; }
        [[nodiscard]] int PendingSpellSlot() const { return pendingSpellSlot_; }
        [[nodiscard]] std::shared_ptr<const Magic::IMagicSpell> PendingSpell() const { return pendingSpell_; }

    private:
        const std::shared_ptr<MagicCasterAvatarStatus     > status_;
        const std::weak_ptr  <GameObject::IGameObject     > playerAvatarObject_;
        const std::weak_ptr  <IPlayerAvatar               > playerAvatar_;
        const std::shared_ptr<MagicCasterAvatarInputAction> inputAction_;
        const std::weak_ptr  <PlayerAvatarCameraGroupBase > cameraGroup_;
        const std::weak_ptr  <GameObject::IGameObject     > castPoint_;
        const std::weak_ptr<Asset::MagicCasterAvatarResource> resources_;
        const std::weak_ptr<Magic::IMagicCaster> caster_;
        const std::weak_ptr<GamePlay::PlayerAvatar::LockOnDetectionArea> lockOnDetectionArea_;
        int pendingSpellSlot_ = 0;
        std::shared_ptr<const Magic::IMagicSpell> pendingSpell_;
    };
}
