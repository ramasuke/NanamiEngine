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

    if (During_secs() < Status().DamageStateDuration_secs())
        return;

    OnChangeState(MagicCasterAvatarStateType::Idle);
}

void GameCore::PlayerAvatar::MagicCaster::State::HurtState::DoExit()
{
}
