#include "SwordManAvatarIdleState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
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
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));

    if (Status().IsDamaged())
        OnChangeState(SwordManAvatarStateType::Hurt);
    if (Input().Move().IsUpdatePressed())
        OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
    if (Input().Jump().IsPressed())
        OnChangeState(SwordManAvatarStateType::Jump);
    if (Input().AvoidRolling().IsPressed() && Status().CanAvoidRolling())
        OnChangeState(SwordManAvatarStateType::AvoidRolling);
    UpdateLockOn();
    if (Input().NormalAttack().IsPressed())
        OnChangeState(SwordManAvatarStateType::NormalAttack);
    if (Conditions().CanWakeUp() && Input().Chat().IsPressed())
        OnChangeState(SwordManAvatarStateType::WakeUp);
    else if (Conditions().IsChattable() && Input().Chat().IsPressed())
        OnChangeState(SwordManAvatarStateType::Chatting);
    if (Conditions().CanUseCannon())
        OnChangeState(SwordManAvatarStateType::UseCanon);
    if (!Conditions().IsGround())
        OnChangeState(SwordManAvatarStateType::Floating);
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoExit()
{

}
