#include "SwordManAvatar_AttackedShockedState.h"

#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Floating/FloatingState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../Run/SwordManAvatarRunState.h"
#include "../Walk/SwordManAvatarWalkState.h"

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoUpdate()
{
    //Change State
    if (Status().AttackedShockedStateDuration_secs() <= During_secs())
    {
        if (Status().IsOnDisableReinforceMode())
            OnChangeState<OnDisableReinforceState>();
        if (!Input().Move().IsUpdatePressed())
            OnChangeState<SwordManAvatarIdleState>();
        if (Input().Move().IsUpdatePressed())
            OnChangeState<SwordManAvatarWalkState>();
        if (Input().Run().IsUpdatePressed())
            OnChangeState<SwordManAvatarRunState>();
        if (Input().Jump().IsPressed())
            OnChangeState<SwordManAvatarJumpState>();
        if (Input().AvoidRolling().IsPressed())
            OnChangeState<AvoidRollingState>();
        if (Input().NormalAttack().IsPressed())
            OnChangeState<SwordManAvatarNormalAttackState>();
        if (!Conditions().IsGround())
            OnChangeState<FloatingState>();
    }   
}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoExit()
{
    
}
