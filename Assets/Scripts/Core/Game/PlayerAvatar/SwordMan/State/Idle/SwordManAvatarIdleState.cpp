#include "SwordManAvatarIdleState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoEnter()
{
    ChangeCamera(CameraGroup().FollowFromBehind());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoFixedUpdate()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));

    if (Status().IsDamaged())
        OnChangeState(SwordManAvatarStateType::Hurt);
    if (Status().IsOnDisableReinforceMode())
        OnChangeState(SwordManAvatarStateType::OnDisableReinforce);
    if (Input().Move().IsUpdatePressed())
        OnChangeState(SwordManAvatarStateType::Walk);
    if (Input().Jump().IsPressed())
        OnChangeState(SwordManAvatarStateType::Jump);
    if (Input().AvoidRolling().IsPressed())
        OnChangeState(SwordManAvatarStateType::AvoidRolling);
    if (Input().NormalAttack().IsPressed())
        OnChangeState(SwordManAvatarStateType::NormalAttack);
    if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
        OnChangeState(SwordManAvatarStateType::OnEnableReinforce);
    if (Conditions().IsChattable() && Input().Chat().IsPressed())
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
