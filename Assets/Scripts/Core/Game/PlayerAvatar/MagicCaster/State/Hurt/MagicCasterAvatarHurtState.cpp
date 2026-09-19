#include "MagicCasterAvatarHurtState.h"

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::DoEnter()
{
    HoldHorizontalVelocity();
    Status().ApplyDamage();

    if (Status().IsDeath())
        OnChangeState(MagicCasterAvatarStateType::Death);
}

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::DoUpdate()
{
    Status().DiscardDamage();
    UpdateTransitions();
}

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::VisitTransitions(
    IMagicCasterAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(MagicCasterAvatarStateType::Idle, During_secs() >= Status().DamageStateDuration_secs());
}

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::DoExit()
{
}
