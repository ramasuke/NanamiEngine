#include "SwordManAvatar_AttackedShockedState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoEnter()
{

}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
    UpdateTransitions();
}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
{
    if (During_secs() < Status().AttackedShockedStateDuration_secs())
        return;

    visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, SwordManAvatarInputPhase::NotHolding, true);
    visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                    SwordManAvatarInput::Move, SwordManAvatarInputPhase::Holding, true);
    visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                    SwordManAvatarInput::Run, SwordManAvatarInputPhase::Holding, Status().CanRun());
    visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, SwordManAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, SwordManAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, true);
    visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoExit()
{

}
