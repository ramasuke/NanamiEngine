#include "MagicCasterAvatarStateContext.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../Status/MagicCasterAvatarStatus.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStateContext::MagicCasterAvatarStateContext(
        const std::shared_ptr<MagicCasterAvatarStatus     >& status      ,
        const std::shared_ptr<MagicCasterAvatarInputAction>& inputAction ,
        const std::weak_ptr  <GameObject::IGameObject     >& playerAvatar,
        const std::weak_ptr  <PlayerAvatarCameraGroupBase >& cameraGroup ,
        const std::weak_ptr  <GameObject::IGameObject     >& castPoint   ,
        const std::weak_ptr  <Asset::MagicCasterAvatarResource>& resources,
        const std::weak_ptr  <Magic::IMagicCaster             >& caster,
        const std::weak_ptr  <GamePlay::PlayerAvatar::LockOnDetectionArea>& lockOnDetectionArea
        )
        : status_             (status             )
        , playerAvatarObject_ (playerAvatar       )
        , playerAvatar_       (playerAvatar.lock()->Components().Catch<IPlayerAvatar>())
        , inputAction_        (inputAction        )
        , cameraGroup_        (cameraGroup        )
        , castPoint_          (castPoint          )
        , resources_          (resources          )
        , caster_             (caster             )
        , lockOnDetectionArea_(lockOnDetectionArea)
    {
    }

    float MagicCasterAvatarStateContext::GroundCheckRadius() const
    {
        return resources_.lock()->GroundCheckRadius();
    }

    float MagicCasterAvatarStateContext::GroundCheckUpOffset() const
    {
        return resources_.lock()->GroundCheckUpOffset();
    }

    float MagicCasterAvatarStateContext::GroundCheckDistance() const
    {
        return resources_.lock()->GroundCheckDistance();
    }

    float MagicCasterAvatarStateContext::MaxWalkableSlope_deg() const
    {
        return resources_.lock()->MaxWalkableSlope_deg();
    }

    float MagicCasterAvatarStateContext::SlopeCheckRadius() const
    {
        return resources_.lock()->SlopeCheckRadius();
    }

    float MagicCasterAvatarStateContext::SlopeCheckUpOffset() const
    {
        return resources_.lock()->SlopeCheckUpOffset();
    }

    float MagicCasterAvatarStateContext::SlopeCheckDistance() const
    {
        return resources_.lock()->SlopeCheckDistance();
    }
}
