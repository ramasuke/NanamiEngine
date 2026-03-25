#include "OnDisableReinforceState.h"

#include "../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../Idle/SwordManAvatarIdleState.h"

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoEnter()
{
    Status().OnDisableReinforce();
    ReinforcingParticle().SetEnable(false);
}

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoUpdate()
{
    if (During_secs() > Status().OnDisableReinforceDuration_secs())
    {
        OnReinforceParticle().SetEnable(false);
        
        auto& reinforceParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::REINFORCE_PARTICLE_NAME);
        reinforceParticle.SetEnable(false);
        OnChangeState<SwordManAvatarIdleState>();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::OnDisableReinforceState::DoExit()
{
    
}
