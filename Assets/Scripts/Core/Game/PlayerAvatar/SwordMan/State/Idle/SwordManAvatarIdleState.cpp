#include "SwordManAvatarIdleState.h"

#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoEnter()
{
    if (!ExpiredCamera())
    {
        // ロックオン中はロックオンカメラを維持する（攻撃・移動から Idle に戻っても解除しない）
        ChangeCamera(CameraGroup().IsLockedOn() ? CameraGroup().LockOnCamera() : CameraGroup().FollowFromBehind());
        
        if (CameraGroup().FollowFromBehind().lock())
        {
        CameraGroup()
            .FollowFromBehind().lock()
            ->Components()
            .Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()
            ->SetEnableLockMousePos(true);
        }
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
                    SwordManAvatarInput::Move, SwordManAvatarInputPhase::Holding, true);
    visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, SwordManAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, SwordManAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    VisitLockOnAction(visitor);
    visitor.Action(SwordManAvatarStateAction::CycleItem, true);
    visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
    visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, true);
    
    const bool canWakeUp = Conditions().CanWakeUp();
    visitor.OnInput(SwordManAvatarStateType::WakeUp, SwordManAvatarInput::Chat, SwordManAvatarInputPhase::Pressed, canWakeUp);
    visitor.OnInput(SwordManAvatarStateType::Chatting, SwordManAvatarInput::Chat, SwordManAvatarInputPhase::Pressed, !canWakeUp && Conditions().IsChattable());
    visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
    visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoExit()
{

}
