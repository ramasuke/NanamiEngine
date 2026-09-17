#include "SwordManAvatarStateContext.h"

#include "../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
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
        // ポーチはセーブに乗せないので、アバターを組むたびにリソースの初期所持から作り直す
        if (const auto resource = resources.lock())
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
}
