#include "SwordManAvatarIdleState.h"

#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoEnter()
{
    // 攻撃・移動から Idle に戻ってもロックオンは解除しない
    ChangeCameraByLockOn();

    if (!ExpiredCamera() && CameraGroup().FollowFromBehind().lock())
    {
        CameraGroup()
            .FollowFromBehind().lock()
            ->Components()
            .Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()
            ->SetEnableLockMousePos(true);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoFixedUpdate()
{
    HoldHorizontalVelocity();

    UpdateLockOn();
    UpdateItemPouchInput();
    UpdateTransitions();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::VisitTransitions(
    ISwordManAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
    visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                    SwordManAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
    visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, PlayerAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    VisitLockOnAction(visitor);
    visitor.Action(SwordManAvatarStateAction::CycleItem, true);
    visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
    visitor.Action(SwordManAvatarStateAction::OpenMenu, true);
    visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::Pressed, true);
    
    const bool canWakeUp = Conditions().CanWakeUp();
    visitor.OnInput(SwordManAvatarStateType::WakeUp, SwordManAvatarInput::Chat, PlayerAvatarInputPhase::Pressed, canWakeUp);
    visitor.OnInput(SwordManAvatarStateType::Chatting, SwordManAvatarInput::Chat, PlayerAvatarInputPhase::Pressed, !canWakeUp && Conditions().IsInteractable());
    visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
    visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoExit()
{

}
