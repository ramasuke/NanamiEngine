#include "SwordManAvatarStateMachine.h"

#include "../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    std::unique_ptr<SwordManAvatarStateMachine> SwordMan::CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup)
    {
        return std::make_unique<SwordManAvatarStateMachine>(
            std::make_shared<SwordManAvatarStateContext>(
                status,
                input,
                playerAvatar->Entity(),
                cameraGroup,
                playerAvatar->CatchNormalAttackArea(),
                playerAvatar->CatchDashAttackArea(),
                playerAvatar->OnReinforceParticle(),    
                playerAvatar->ReinforcingParticle(),
                playerAvatar->Resources()
            ));
    }
}
