#include "SwordManAvatar_FallDownState.h"


void GameCore::PlayerAvatar::SwordMan::State::FallDownState::DoEnter()
{
    Status().SetDowned(true);
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::FallDownState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::FallDownState::DoUpdate()
{
    if (During_secs() >= Status().FallDownStateDuration_secs())
        OnChangeState(SwordManAvatarStateType::Down);
}

void GameCore::PlayerAvatar::SwordMan::State::FallDownState::DoExit()
{

}
