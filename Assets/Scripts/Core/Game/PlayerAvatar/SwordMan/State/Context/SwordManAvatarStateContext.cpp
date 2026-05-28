#include "SwordManAvatarStateContext.h"

#include "../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStateContext::SwordManAvatarStateContext(
        const std::shared_ptr<SwordManAvatarStatus     >& status          ,
        const std::shared_ptr<SwordManAvatarInputAction>& inputAction     ,
        const std::weak_ptr  <GameObject::IGameObject  >& playerAvatar    ,
        const std::weak_ptr  <SwordManAvatarCameraGroup>& cameraGroup     ,
        const std::weak_ptr  <PlayerAttackArea>& normalAttackArea,
        const std::weak_ptr  <PlayerAttackArea>& dashAttackArea,
        const std::weak_ptr  <Component::ParticleSystem>& onReinforceParticle,
        const std::weak_ptr  <Component::ParticleSystem>& reinforcingParticle,
        const std::weak_ptr<Asset::SwordManAvatarResource>& resources
        )
        : status_             (status             )
        , playerAvatarObject_ (playerAvatar    )
        , playerAvatar_       (playerAvatar.lock()->Components().Catch<IPlayerAvatar>())
        , inputAction_        (inputAction        )
        , cameraGroup_        (cameraGroup     )
        , normalAttackArea_   (normalAttackArea)
        , dashAttackArea_     (dashAttackArea  )
        , onReinforceParticle_(onReinforceParticle)
        , reinforcingParticle_(reinforcingParticle)
        , resources_          (resources          )
    {
        
    }
}
