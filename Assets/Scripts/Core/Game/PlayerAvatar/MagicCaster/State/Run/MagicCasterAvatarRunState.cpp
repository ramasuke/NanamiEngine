#include "MagicCasterAvatarRunState.h"

#include "../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoEnter()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoFixedUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().MoveForward(Status().GetRunSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoUpdate()
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
    if (!Input().Run().IsUpdatePressed() || !Status().CanRun())
    {
        OnChangeState(MagicCasterAvatarStateType::Walk);
        return;
    }
    if (Input().Jump().IsPressed() && Status().CanJump())
    {
        OnChangeState(MagicCasterAvatarStateType::Jump);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoExit()
{
}
