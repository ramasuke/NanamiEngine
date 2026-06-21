#include "OnDisableReinforceState.h"

#include "../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoEnter()
{
    Status().OnDisableReinforce();
    
    // ReinforcingParticle().SetEnable(false);
}

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoFixedUpdate()
{
    if (During_secs() > Status().OnDisableReinforceDuration_secs())
    {
        // OnReinforceParticle().SetEnable(false);

        auto& reinforceParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::REINFORCE_PARTICLE_NAME);
        reinforceParticle.SetEnable(false);
        OnChangeState(SwordManAvatarStateType::Idle);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoExit()
{

}
