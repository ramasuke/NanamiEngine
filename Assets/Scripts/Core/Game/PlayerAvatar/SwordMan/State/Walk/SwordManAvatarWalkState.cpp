#include "SwordManAvatarWalkState.h"

#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Floating/FloatingState.h"
#include "../Hurt/SwordManAvatar_HurtState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../OnEnableReinforce/OnEnableReinforceState.h"
#include "../Run/SwordManAvatarRunState.h"
#include "../UseCanon/SwordManAvatarUseCanonState.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarWalkState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarWalkState::DoUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().ForwardMove(Status().GetWalkSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());

    if (Status().IsDamaged())
        OnChangeState<HurtState>();
    if (Status().IsOnDisableReinforceMode())
        OnChangeState<OnDisableReinforceState>();
    if (!Input().Move().IsUpdatePressed())
        OnChangeState<SwordManAvatarIdleState>();
    if (Input().Run().IsUpdatePressed())
        OnChangeState<SwordManAvatarRunState>();
    if (Input().Jump().IsPressed())
        OnChangeState<SwordManAvatarJumpState>();
    if (Input().AvoidRolling().IsPressed())
        OnChangeState<AvoidRollingState>();
    if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
        OnChangeState<OnEnableReinforceState>();
    if (Input().NormalAttack().IsPressed())
        OnChangeState<SwordManAvatarNormalAttackState>();
    if (Conditions().CanUseCannon())
        OnChangeState<SwordManAvatarUseCannonState>();
    if (!Conditions().IsGround())
        OnChangeState<FloatingState>();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarWalkState::DoExit()
{
    
}
