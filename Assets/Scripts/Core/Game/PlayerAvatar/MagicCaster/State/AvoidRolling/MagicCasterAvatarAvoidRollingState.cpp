#include "MagicCasterAvatarAvoidRollingState.h"

#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoEnter()
{
    isAvoided_ = false;
    Status().ConsumeAvoidRollingStamina();
    if (const auto sound = Resources().AvoidRollingSound())
        GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoFixedUpdate()
{
    // NOTE: 転がっている間の被ダメージは受け流す(捨てる)
    if (Status().IsDamaged())
    {
        if (!isAvoided_)
        {
            if (const auto particle = Context().SuccessAvoidRollingParticle())
                particle->Play();
            if (const auto sound = Resources().JustAvoidRollingSound())
                GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
        }
        Status().DiscardDamage();
        isAvoided_ = true;
    }

    if (Status().AvoidRollingStateDuration_secs() <= During_secs())
        OnChangeState(Input().Move().IsUpdatePressed() ? MagicCasterAvatarStateType::Walk : MagicCasterAvatarStateType::Idle);
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::AvoidRollingState::DoExit()
{
}
