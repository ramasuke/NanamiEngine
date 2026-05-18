#include "SwordManAvatarTraits.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    std::unique_ptr<SwordManAvatarTraits::StateMachine>
    SwordManAvatarTraits::CreateStateMachine(
        const std::shared_ptr<SwordManAvatarStatus     >& status, 
        const std::shared_ptr<SwordManAvatarInputAction>& input, 
        const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar, 
        const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup)
    {
        return std::move(SwordMan::CreateStateMachine(
            status,
            input,
            playerAvatar,
            cameraGroup));
    }
}
