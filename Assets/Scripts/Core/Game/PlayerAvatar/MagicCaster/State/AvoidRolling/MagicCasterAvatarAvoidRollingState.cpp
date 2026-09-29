#include "MagicCasterAvatarAvoidRollingState.h"

#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoEnter()
{
    isAvoided_ = false;
    FaceAvoidRollingDirection();
    Status().ConsumeAvoidRollingStamina();
    if (const auto sound = Resources().AvoidRollingSound())
        GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoFixedUpdate()
{
    MoveAvoidRolling();

    if (Status().IsDamaged())
    {
        if (!isAvoided_ && During_secs() <= Status().JustAvoidWindow_secs())
        {
            if (const auto particle = Context().SuccessAvoidRollingParticle())
                particle->Play();
            
            if (const auto sound = Resources().JustAvoidRollingSound())
                GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
            Status().OnJustAvoided();
            isAvoided_ = true;
        }
        Status().DiscardDamage();
    }

    // ジャスト回避すれば残りの転がりを打ち切ってカウンター魔法を撃てる
    if (isAvoided_ && MIN_ROLL_BEFORE_COUNTER_SECS <= During_secs() && CanCounterCast() && Input().Cast().IsPressed() && TryBeginCast())
        return;

    if (Status().AvoidRollingStateDuration_secs() <= During_secs())
        OnChangeState(Input().Move().IsUpdatePressed() ? MagicCasterAvatarStateType::Walk : MagicCasterAvatarStateType::Idle);
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoExit()
{
}
