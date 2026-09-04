#include "SwordManAvatarStateBase.h"

#include <random>
#include <vector>

#include "../../../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../Chattable/IPlayerChattable.h"

namespace
{
    /** 設定値は適当:  */
    const std::vector DEFAULT_FOOTSTEP_CONTACT_PHASES = { 0.25f, 0.75f };

    /**
     * @brief クリップ正規化時間が`phase`をこのフレームで通過したか
     * @note prev > cur はループ折り返し
     */
    bool CrossedFootstepPhase(const float phase, const float prev, const float cur)
    {
        if (prev < 0.0f)
            return false;
        if (prev <= cur)
            return prev < phase && phase <= cur;
        return phase > prev || phase <= cur;
    }
}

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStateBase::SwordManAvatarStateBase(
        const std::shared_ptr<SwordManAvatarStateContext>& context
        , const std::function<void(SwordManAvatarStateType)>& onChangeState)
        : stateDuring_secs_(0.0f            )
        , context_         (context         )
        , onChangeState_   (onChangeState)
    {
    }
    
    void SwordManAvatarStateBase::OnEnter()
    {
        ResetDuringTime();
        prevFootstepNormalizedTime_ = -1.0f;
        DoEnter();
    }
    
    void SwordManAvatarStateBase::OnUpdate()
    {
        DoUpdate();
        stateDuring_secs_ += Time::DeltaTime();
    }

    void SwordManAvatarStateBase::OnFixedUpdate()
    {
        DoFixedUpdate();
    }

    void SwordManAvatarStateBase::OnExit()
    {
        DoExit();
    }

    Component::Animator& SwordManAvatarStateBase::Animator() const
    {
        return *Player().Components().Catch<Component::Animator>().lock();
    }

    void SwordManAvatarStateBase::TryEmitFootstep(const std::vector<float>& contactPhases,
                                                  const std::vector<FIELD(Asset::SoundFile)>& footstepSounds)
    {
        if (!Resources().HasFootstepParticlePrefab() && footstepSounds.empty())
            return;

        const auto& phases = contactPhases.empty() ? DEFAULT_FOOTSTEP_CONTACT_PHASES : contactPhases;

        const auto progress = Animator().GetCurrentClipProgress();
        if (!progress)
        {
            prevFootstepNormalizedTime_ = -1.0f;
            return;
        }

        const float currentNormalizedTime = progress->normalizedTime;
        for (const float phase : phases)
        {
            if (!CrossedFootstepPhase(phase, prevFootstepNormalizedTime_, currentNormalizedTime))
                continue;

            if (Resources().HasFootstepParticlePrefab())
                Scene::GameObject::Instantiate(Resources().FootstepParticlePrefab(), FeatStepPos());

            if (!footstepSounds.empty())
            {
                static std::mt19937 footstepRng{ std::random_device{}() };
                std::uniform_int_distribution<size_t> pick(0, footstepSounds.size() - 1);
                if (const auto footstepSound = footstepSounds[pick(footstepRng)].get())
                    GamePlay::Sound::SoundPlayer::PlaySe(*footstepSound, FeatStepPos());
            }
            break;
        }
        prevFootstepNormalizedTime_ = currentNormalizedTime;
    }

    void SwordManAvatarStateBase::ResetDuringTime()
    {
        stateDuring_secs_ = 0.0f;
    }

    void SwordManAvatarStateBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
    {
        CameraGroup().ChangeCamera(camera);
    }

    void SwordManAvatarStateBase::OnChangeState(SwordManAvatarStateType type) const
    {
        onChangeState_(type);
    }

    void SwordManAvatarStateBase::OnTryChangeState(
        SwordManAvatarStateType type,
        const std::function<bool()>& check) const
    {
        if (check())
            OnChangeState(type);
    }

    void SwordManAvatarStateBase::OnTryChangeState(
        const SwordManAvatarStateType type,
        const bool check) const
    {
        if (check)
            OnChangeState(type);
    }
}
