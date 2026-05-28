#include "SwordManAvatar_AvoidRolling.h"

#include "../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../Idle/SwordManAvatarIdleState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void AvoidRollingState::DoEnter()
    {
        isAvoided_ = false;
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().AvoidRollingSound(), Transform().GetWorldPos());
        StatusEvent().InvokeOnAvoidRolling();
    }

    void AvoidRollingState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            if (!isAvoided_)
            {
                auto& successAvoidRollingParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::SUCCESS_AVOID_ROLLING_PARTICLE_NAME);
                successAvoidRollingParticle.Play();
                GamePlay::Sound::SoundPlayer::PlaySe(Resources().JustAvoidRollingSound(), Transform().GetWorldPos());   
            }
            Status().DiscardDamage();
            isAvoided_ = true;
        }
        
        if (Status().AvoidRollingStateDuration_secs() <= During_secs())
        {
            if (Status().IsDamaged())
            {
                OnChangeState<HurtState>();
            }
            if (Input().Move().IsUpdatePressed())
            {
                OnChangeState<SwordManAvatarWalkState>();
            }
            else
            {
                OnChangeState<SwordManAvatarIdleState>();
            }
        }
    }

    void AvoidRollingState::DoExit()
    {
        
    }
}
