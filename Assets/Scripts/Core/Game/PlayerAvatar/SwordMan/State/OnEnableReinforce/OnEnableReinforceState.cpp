#include "OnEnableReinforceState.h"

#include "../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

void GameCore::PlayerAvatar::SwordMan::State::OnEnableReinforceState::DoEnter()
{
    Status().OnEnableReinforce();
    auto& reinforceParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::REINFORCE_PARTICLE_NAME);
    reinforceParticle.SetEnable(true);
}

void GameCore::PlayerAvatar::SwordMan::State::OnEnableReinforceState::DoFixedUpdate()
{
    if (During_secs() > Status().OnEnableReinforceDuration_secs())
    {
        auto& onReinforceParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::ON_REINFORCE_PARTICLE_NAME);
        onReinforceParticle.SetEnable(true);
        auto& reinforcingParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::REINFORCING_PARTICLE_NAME);
        reinforcingParticle.SetEnable(true);
        auto& reinforceParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::REINFORCE_PARTICLE_NAME);
        reinforceParticle.SetEnable(true);
        OnChangeState(SwordManAvatarStateType::Idle);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::OnEnableReinforceState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::OnEnableReinforceState::DoExit()
{

}
