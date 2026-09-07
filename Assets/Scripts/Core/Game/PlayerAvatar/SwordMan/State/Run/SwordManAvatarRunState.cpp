#include "SwordManAvatarRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoEnter()
{
    StatusEvent().InvokeOnRun();
    Status().SetIsRunning(true);
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoFixedUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().ForwardMove(Status().GetRunSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
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
    if (Input().Jump().IsPressed())
        OnChangeState(SwordManAvatarStateType::Jump);
    if (Input().AvoidRolling().IsPressed())
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
