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

        // NOTE: 転がっている間の被ダメージはずっと受け流す。報酬は出だしの窓で受け流した時だけ
        if (Status().IsDamaged())
        {
            if (!isAvoided_ && During_secs() <= Status().JustAvoidWindow_secs())
            {
                SuccessAvoidRollingParticle().Play();
                GamePlay::Sound::SoundPlayer::PlaySe(Resources().JustAvoidRollingSound(), Transform().GetWorldPos());
                Status().OnJustAvoided();
                isAvoided_ = true;
            }
            Status().DiscardDamage();
        }

        // NOTE: ジャスト回避が決まっていれば、残りの転がりを打ち切って反撃できる
        if (isAvoided_ && Status().CanCounter() && MIN_ROLL_BEFORE_COUNTER_SECS <= During_secs() && Input().NormalAttack().IsPressed())
        {
            OnChangeState(SwordManAvatarStateType::CounterAttack);
            return;
        }

        if (Status().AvoidRollingStateDuration_secs() <= During_secs())
        {
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
