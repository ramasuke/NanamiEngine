#include "SwordManAvatarStateBase.h"

#include <random>
#include <vector>

#include "../../../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../Chattable/IPlayerChattable.h"
#include "../../Input/PlayerAvatarInput_void.h"

namespace
{
    /** 設定値は適当:  */
    const std::vector DEFAULT_FOOTSTEP_CONTACT_PHASES = { 0.25f, 0.75f };

    // Target側コライダー表面での取りこぼし（浮動小数誤差）を避けるための余白
    constexpr float LOCK_ON_LOS_RAY_MARGIN = 1.0f;

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
        TickHitStop();
        DoUpdate();
        stateDuring_secs_ += Time::DeltaTime();
    }

    void SwordManAvatarStateBase::OnFixedUpdate()
    {
        DoFixedUpdate();
    }

    void SwordManAvatarStateBase::OnExit()
    {
        // 状態遷移を跨いでtimeScaleが下がったまま残らないよう、念のため強制復帰する
        if (hitStopRemaining_secs_ > 0.0f)
        {
            hitStopRemaining_secs_ = 0.0f;
            Animator().SetTimeScale(1.0f);
        }
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

    void SwordManAvatarStateBase::TickHitStop()
    {
        if (hitStopRemaining_secs_ <= 0.0f)
            return;

        hitStopRemaining_secs_ -= Time::DeltaTime();
        if (hitStopRemaining_secs_ <= 0.0f)
            Animator().SetTimeScale(1.0f);
    }

    void SwordManAvatarStateBase::TriggerHitStop(const float duration_secs, const float timeScale)
    {
        hitStopRemaining_secs_ = duration_secs;
        Animator().SetTimeScale(timeScale);
    }

    void SwordManAvatarStateBase::DealDamageText(PlayerAttackArea& attackArea, const Damage::PhysicsPower power) const
    {
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        for (const auto& attackTarget : attackArea.Targets())
        {
            const auto origin    = Transform().GetWorldPos();
            const auto targetPos = attackTarget.GameObject().Transform().GetWorldPos();
            const auto direction = targetPos - origin;

            const auto raycastHit = Physics::Raycast(
                                            origin,
                                            direction,
                                            glm::length(direction),
                                            mask);

            const auto textPos = raycastHit.Hit() ? raycastHit.Position() : targetPos;
            const auto damageText = Scene::GameObject::Instantiate(Resources().DealDamageTextBillBoardPrefab(), textPos);
            damageText.lock()->Components().Catch<GamePlay::Ui::DealDamageTextBillBoard>().lock()->Play(power.Value());
        }
    }

    void SwordManAvatarStateBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
    {
        CameraGroup().ChangeCamera(camera);
    }

    void SwordManAvatarStateBase::UpdateLockOn() const
    {
        if (CameraGroup().IsLockedOn() && !IsLockOnTargetInRange())
            CameraGroup().ReleaseLockOn();

        if (!Input().LockOn().IsPressed())
            return;

        if (CameraGroup().IsLockedOn())
        {
            CameraGroup().ReleaseLockOn();
            return;
        }

        if (const auto target = FindNearestLockOnTarget())
            CameraGroup().EngageLockOn(target);
    }

    void SwordManAvatarStateBase::RotateTowardsLockOnTarget(const float rotateSpeed) const
    {
        if (ExpiredCamera())
            return;

        // 対象が死亡して weak_ptr が切れた場合も lock() で吸収する
        const auto target = CameraGroup().LockOnTarget().lock();
        if (!target)
            return;

        const glm::vec3 toTarget = target->Transform().GetWorldPos() - Transform().GetWorldPos();
        Actions().RotateTowards(glm::vec3(toTarget.x, 0.0f, toTarget.z), rotateSpeed);
    }

    bool SwordManAvatarStateBase::IsLockOnTargetInRange() const
    {
        const auto currentTarget = CameraGroup().LockOnTarget().lock();
        if (!currentTarget)
            return false;

        for (const auto& candidate : LockOnDetectionArea().Candidates())
            if (candidate.lock() == currentTarget)
                return HasLineOfSight(currentTarget); // 索敵範囲内でも遮蔽されたら解除
        return false; // 索敵範囲外に出た
    }

    std::shared_ptr<GameObject::IGameObject> SwordManAvatarStateBase::FindNearestLockOnTarget() const
    {
        std::shared_ptr<GameObject::IGameObject> nearestTarget;
        float nearestDistanceSq = -1.0f;
        const auto playerPos = Transform().GetWorldPos();

        for (const auto& weakCandidate : LockOnDetectionArea().Candidates())
        {
            const auto candidate = weakCandidate.lock();
            if (!candidate)
                continue;
            if (!HasLineOfSight(candidate))
                continue;

            const glm::vec3 diff = candidate->Transform().GetWorldPos() - playerPos;
            const float distanceSq = glm::dot(diff, diff);
            if (nearestDistanceSq < 0.0f || distanceSq < nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                nearestTarget = candidate;
            }
        }
        return nearestTarget;
    }

    bool SwordManAvatarStateBase::HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target) const
    {
        const glm::vec3 origin = CineMachine::CinemachineCameraBrain::Instance()->Transform().GetWorldPos();
        const glm::vec3 targetPos = target->Transform().GetWorldPos();
        const glm::vec3 diff = targetPos - origin;
        const float distance = glm::length(diff);
        if (distance <= 0.0f)
            return true;

        // Playerのみ除外（Default・Enemyは視線を遮る対象として扱う）
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);
        Physics::AddLayer(mask, Physics::Layer::Enemy);

        const auto hit = Physics::Raycast(origin, diff, distance + LOCK_ON_LOS_RAY_MARGIN, mask);
        if (!hit.Hit())
            return false; // 何にも当たらなかった＝対象自体にも当たっていない異常系。安全側に倒す

        return &hit.HitObject() == target.get();
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
