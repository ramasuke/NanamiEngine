#include "SwordManAvatar_AvoidRolling.h"

#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void AvoidRollingState::DoEnter()
    {
        isAvoided_ = false;
        FaceAvoidRollingDirection();
        Status().ConsumeAvoidRollingStamina();
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().AvoidRollingSound(), Transform().GetWorldPos());
        StatusEvent().InvokeOnAvoidRolling();
    }

    void AvoidRollingState::DoFixedUpdate()
    {
        MoveAvoidRolling();

        // NOTE: 転がっている間の被ダメージはずっと受け流す。演出は出だしの窓で受け流した時だけ
        if (Status().IsDamaged())
        {
            if (!isAvoided_ && During_secs() <= Status().JustAvoidWindow_secs())
            {
                SuccessAvoidRollingParticle().Play();
                GamePlay::Sound::SoundPlayer::PlaySe(Resources().JustAvoidRollingSound(), Transform().GetWorldPos());
                isAvoided_ = true;
            }
            Status().DiscardDamage();
        }

        if (Status().AvoidRollingStateDuration_secs() <= During_secs())
        {
            if (Status().IsDamaged())
            {
                OnChangeState(SwordManAvatarStateType::Hurt);
            }
            if (Input().Move().IsUpdatePressed())
            {
                OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
            }
            else
            {
                OnChangeState(SwordManAvatarStateType::Idle);
            }
        }
    }

    void AvoidRollingState::DoUpdate()
    {

    }

    void AvoidRollingState::DoExit()
    {

    }
}
