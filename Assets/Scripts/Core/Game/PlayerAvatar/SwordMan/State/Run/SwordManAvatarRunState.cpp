#include "SwordManAvatarRunState.h"

#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Dash/SwordManAvatarDashAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Floating/FloatingState.h"
#include "../Hurt/SwordManAvatar_HurtState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../OnEnableReinforce/OnEnableReinforceState.h"
#include "../Walk/SwordManAvatarWalkState.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().ForwardMove(Status().GetRunSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());

    if (Status().IsDamaged())
        OnChangeState<HurtState>();
    if (Status().IsOnDisableReinforceMode())
        OnChangeState<OnDisableReinforceState>();
    if (!Input().Move().IsUpdatePressed())
        OnChangeState<SwordManAvatarIdleState>();
    if (!Input().Run().IsUpdatePressed())
        OnChangeState<SwordManAvatarWalkState>();
    if (Input().Jump().IsPressed())
        OnChangeState<SwordManAvatarJumpState>();
    if (Input().AvoidRolling().IsPressed())
        OnChangeState<AvoidRolling>();
    if (Input().DashAttack().IsPressed())
        OnChangeState<SwordManAvatarDashAttackState>();
    if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
        OnChangeState<OnEnableReinforceState>();
    if (!Conditions().IsGround())
        OnChangeState<FloatingState>();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoExit()
{
    
}