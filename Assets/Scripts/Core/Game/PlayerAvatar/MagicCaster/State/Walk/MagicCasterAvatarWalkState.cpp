#include "MagicCasterAvatarWalkState.h"

#include "../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::WalkState::DoEnter()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::WalkState::DoFixedUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().MoveForward(Status().GetWalkSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
}

void GameCore::PlayerAvatar::MagicCaster::State::WalkState::DoUpdate()
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
    if (!Input().Move().IsUpdatePressed())
    {
        OnChangeState(MagicCasterAvatarStateType::Idle);
        return;
    }
    if (Input().Run().IsUpdatePressed() && Status().CanRun())
    {
        OnChangeState(MagicCasterAvatarStateType::Run);
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
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::WalkState::DoExit()
{
}
