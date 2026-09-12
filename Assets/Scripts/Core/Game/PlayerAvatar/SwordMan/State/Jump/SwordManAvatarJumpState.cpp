#include "SwordManAvatarJumpState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpState::DoEnter()
    {
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().JumpSound(), Transform().GetWorldPos());
        Actions().Jump(glm::vec3{0, 1, 0} * Status().GetJumpPower());
        Status().StartJumpCooldown();
    }

    void SwordManAvatarJumpState::DoFixedUpdate()
    {
        if (During_secs() > Status().GetJumpStateDuration_secs())
        {
            OnChangeState(SwordManAvatarStateType::Floating);
        }
    }

    void SwordManAvatarJumpState::DoUpdate()
    {

    }

    void SwordManAvatarJumpState::DoExit()
    {

    }
}
