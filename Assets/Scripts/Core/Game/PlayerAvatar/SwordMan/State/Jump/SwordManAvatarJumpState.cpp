#include "SwordManAvatarJumpState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../Floating/FloatingState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpState::DoEnter()
    {
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().JumpSound(), Transform().GetWorldPos());
        Actions().Jump(glm::vec3{0, 1, 0} * Status().GetJumpPower());
    }

    void SwordManAvatarJumpState::DoUpdate()
    {
        if (During_secs() > Status().GetJumpCooldown_secs())
        {
            OnChangeState<FloatingState>();
        }
    }

    void SwordManAvatarJumpState::DoExit()
    {
        
    }
}
