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
        const std::weak_ptr  <Asset::MagicCasterAvatarResource>& resources
        )
        : status_            (status             )
        , playerAvatarObject_(playerAvatar       )
        , playerAvatar_      (playerAvatar.lock()->Components().Catch<IPlayerAvatar>())
        , inputAction_       (inputAction        )
        , cameraGroup_       (cameraGroup        )
        , castPoint_         (castPoint          )
        , resources_         (resources          )
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
}
