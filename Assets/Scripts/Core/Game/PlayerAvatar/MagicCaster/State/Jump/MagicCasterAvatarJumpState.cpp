#include "MagicCasterAvatarJumpState.h"

void GameCore::PlayerAvatar::MagicCaster::State::JumpState::DoEnter()
{
    ApplyJump();
}

void GameCore::PlayerAvatar::MagicCaster::State::JumpState::DoFixedUpdate()
{
    if (During_secs() > Status().GetJumpStateDuration_secs())
    {
        OnChangeState(MagicCasterAvatarStateType::Floating);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::JumpState::DoUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::JumpState::DoExit()
{
}
