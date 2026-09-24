#include "SwordManAvatarJumpState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpState::DoEnter()
    {
        StatusEvent().InvokeOnJump();
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().JumpSound(), Transform().GetWorldPos());
        ApplyJump();
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
        UpdateTransitions();
    }

    void SwordManAvatarJumpState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.OnInput(SwordManAvatarStateType::JumpAttackAir, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::Pressed, !Conditions().IsGround(Resources().JumpAttackGroundCheckRadius()));
    }

    void SwordManAvatarJumpState::DoExit()
    {

    }
}
