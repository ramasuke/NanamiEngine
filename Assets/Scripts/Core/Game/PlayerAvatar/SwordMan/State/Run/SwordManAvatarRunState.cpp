#include "SwordManAvatarRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoEnter()
{
    StatusEvent().InvokeOnRun();
    Status().SetIsRunning(true);
    ResetMoveSpeedFromVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoFixedUpdate()
{
    AcceleratedForwardMove(Status().GetRunSpeed(), Status().RunAccelerationTime_secs());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoUpdate()
{
    TryEmitFootstep(Resources().RunFootstepContactPhases(), Resources().RunFootstepSounds());

    if (Status().IsInjured())
        OnChangeState(SwordManAvatarStateType::InjuredRun);
    if (Status().IsDamaged())
        OnChangeState(SwordManAvatarStateType::Hurt);
    if (!Input().Move().IsUpdatePressed())
        OnChangeState(SwordManAvatarStateType::Idle);
    if (!Input().Run().IsUpdatePressed() || !Status().CanRun())
        OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
    if (Input().Jump().IsPressed() && Status().CanJump())
        OnChangeState(SwordManAvatarStateType::Jump);
    if (Input().AvoidRolling().IsPressed() && Status().CanAvoidRolling())
        OnChangeState(SwordManAvatarStateType::AvoidRolling);
    UpdateLockOn();
    if (Input().DashAttack().IsPressed())
        OnChangeState(SwordManAvatarStateType::DashAttack);
    if (Conditions().CanUseCannon())
        OnChangeState(SwordManAvatarStateType::UseCanon);
    if (!Conditions().IsGround())
        OnChangeState(SwordManAvatarStateType::Floating);
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoExit()
{
    Status().SetIsRunning(false);
}
