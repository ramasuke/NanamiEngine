#include "SwordManAvatar_HurtState.h"

#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../GamePlay/Ui/PlayerStatus/Ui_DamageFlash.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoEnter()
{
    NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera();
    GamePlay::Ui::DamageFlashUI::FlashMainScreen();

    HoldHorizontalVelocity();
    Status().ApplyDamage();
    StatusEvent().InvokeOnDamage(Status().Health());

    if (Status().IsDeath())
        OnChangeState(SwordManAvatarStateType::FallDown);
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoUpdate()
{
    Status().DiscardDamage();

    UpdateTransitions();
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
{
    if (During_secs() < Status().DamageStateDuration_secs())
        return;

    visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, PlayerAvatarInputPhase::NotHolding, true);
    visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                    SwordManAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
    visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                    SwordManAvatarInput::Run, PlayerAvatarInputPhase::Holding, Status().CanRun());
    visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, PlayerAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::Pressed, true);
    visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoExit()
{

}
