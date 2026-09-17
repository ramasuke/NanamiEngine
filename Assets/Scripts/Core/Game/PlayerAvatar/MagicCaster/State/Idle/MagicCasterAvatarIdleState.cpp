#include "MagicCasterAvatarIdleState.h"

#include "../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoEnter()
{
    if (!ExpiredCamera())
        ChangeCamera(CameraGroup().FollowFromBehind());
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoUpdate()
{
    if (Status().IsDamaged())
    {
        OnChangeState(MagicCasterAvatarStateType::Hurt);
        return;
    }
    if (!Conditions().IsGround())
    {
        OnChangeState(MagicCasterAvatarStateType::Floating);
        return;
    }
    if (Input().Jump().IsPressed() && Status().CanJump())
    {
        OnChangeState(MagicCasterAvatarStateType::Jump);
        return;
    }
    if (Input().Cast().IsPressed() && Status().CanCast())
    {
        OnChangeState(MagicCasterAvatarStateType::Cast);
        return;
    }
    if (Input().Move().IsUpdatePressed())
    {
        OnChangeState(MagicCasterAvatarStateType::Walk);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoExit()
{
}
