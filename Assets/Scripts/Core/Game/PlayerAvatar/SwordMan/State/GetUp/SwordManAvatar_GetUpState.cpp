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
    // NOTE: 起き上がり中は無敵。食らったダメージを Idle へ持ち越さない
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
