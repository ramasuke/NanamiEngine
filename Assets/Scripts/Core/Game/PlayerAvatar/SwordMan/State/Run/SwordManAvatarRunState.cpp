#include "SwordManAvatarRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoEnter()
{
    StatusEvent().InvokeOnRun();
    ResetMoveSpeedFromVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoFixedUpdate()
{
    MoveForward(Status().GetRunSpeed(), Resources().RunAccelerationTime_secs(), Resources().RunDecelerationTime_secs());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoUpdate()
{
    TryEmitFootstep(Resources().RunFootstepSounds());

    UpdateLockOn();
    UpdateItemPouchInput();
    UpdateTransitions();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(SwordManAvatarStateType::InjuredRun, Status().IsInjured());
    visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
    visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, SwordManAvatarInputPhase::NotHolding, true);
    visitor.Automatic(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                      !Input().Run().IsUpdatePressed() || !Status().CanRun());
    visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, SwordManAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, SwordManAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    visitor.Action(SwordManAvatarStateAction::Move, true);
    VisitLockOnAction(visitor);
    visitor.Action(SwordManAvatarStateAction::CycleItem, true);
    visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
    visitor.OnInput(SwordManAvatarStateType::DashAttack, SwordManAvatarInput::DashAttack, SwordManAvatarInputPhase::Pressed, true);
    visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
    visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarRunState::DoExit()
{
}
