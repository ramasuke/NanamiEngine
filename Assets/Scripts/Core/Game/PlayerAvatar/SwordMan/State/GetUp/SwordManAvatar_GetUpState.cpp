#include "SwordManAvatar_GetUpState.h"


void GameCore::PlayerAvatar::SwordMan::State::GetUpState::DoEnter()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::GetUpState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::GetUpState::DoUpdate()
{
    // Invulnerable while getting up: drop hits instead of carrying them into Idle.
    Status().DiscardDamage();

    UpdateTransitions();
}

void GameCore::PlayerAvatar::SwordMan::State::GetUpState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
{
    if (During_secs() < Status().GetUpStateDuration_secs())
        return;

    visitor.Automatic(SwordManAvatarStateType::Idle, true);
}

void GameCore::PlayerAvatar::SwordMan::State::GetUpState::DoExit()
{

}
