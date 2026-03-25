#include "SwordManAvatar_AvoidRolling.h"

#include "../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../Idle/SwordManAvatarIdleState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void AvoidRolling::DoEnter()
    {
        isAvoided_ = false;
        GamePlay::Sound::SoundPlayer::PlaySe(AvoidRollingSound(), Transform().GetWorldPos());
    }

    void AvoidRolling::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            if (!isAvoided_)
            {
                auto& successAvoidRollingParticle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::SUCCESS_AVOID_ROLLING_PARTICLE_NAME);
                successAvoidRollingParticle.Play();
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

    void AvoidRolling::DoExit()
    {
        
    }
}
