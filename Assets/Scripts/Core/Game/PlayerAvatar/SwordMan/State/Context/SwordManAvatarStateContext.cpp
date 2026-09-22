#include "SwordManAvatarStateContext.h"

#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../Status/SwordManAvatarStatus.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStateContext::SwordManAvatarStateContext(
        const std::shared_ptr<SwordManAvatarStatus     >& status          ,
        const std::shared_ptr<SwordManAvatarInputAction>& inputAction     ,
        const std::weak_ptr  <GameObject::IGameObject  >& playerAvatar    ,
        const std::weak_ptr  <SwordManAvatarCameraGroup>& cameraGroup     ,
        const std::weak_ptr  <PlayerAttackArea>& normalAttackArea,
        const std::weak_ptr  <PlayerAttackArea>& dashAttackArea,
        const std::weak_ptr  <GamePlay::PlayerAvatar::LockOnDetectionArea>& lockOnDetectionArea,
        const std::weak_ptr  <Component::ParticleSystem>& successAvoidRollingParticle,
        const std::weak_ptr  <Asset::SwordManAvatarResource>& resources
        )
        : status_               (status             )
        , playerAvatarObject_   (playerAvatar    )
        , playerAvatar_         (playerAvatar.lock()->Components().Catch<IPlayerAvatar>())
        , inputAction_          (inputAction        )
        , cameraGroup_          (cameraGroup     )
        , normalAttackArea_     (normalAttackArea)
        , dashAttackArea_       (dashAttackArea  )
        , lockOnDetectionArea_  (lockOnDetectionArea)
        , successAvoidRollingParticle_(successAvoidRollingParticle)
        , resources_            (resources          )
    {
        // 初期所持はセーブにポーチが無いとき(初回・v21 より前のセーブ)だけ入れる
        if (const auto resource = resources.lock(); resource && !status->Pouch().IsSetUp())
            status->SetupPouch(resource->InitialItems());
    }

    float SwordManAvatarStateContext::GroundCheckRadius() const
    {
        return resources_.lock()->GroundCheckRadius();
    }

    float SwordManAvatarStateContext::GroundCheckUpOffset() const
    {
        return resources_.lock()->GroundCheckUpOffset();
    }

    float SwordManAvatarStateContext::GroundCheckDistance() const
    {
        return resources_.lock()->GroundCheckDistance();
    }

    float SwordManAvatarStateContext::MaxWalkableSlope_deg() const
    {
        return resources_.lock()->MaxWalkableSlope_deg();
    }

    float SwordManAvatarStateContext::SlopeCheckRadius() const
    {
        return resources_.lock()->SlopeCheckRadius();
    }

    float SwordManAvatarStateContext::SlopeCheckUpOffset() const
    {
        return resources_.lock()->SlopeCheckUpOffset();
    }

    float SwordManAvatarStateContext::SlopeCheckDistance() const
    {
        return resources_.lock()->SlopeCheckDistance();
    }
}
