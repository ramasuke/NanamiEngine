#include "SwordManAvatarStateBase.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

#include "../../../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../../../Engine/Core/Application/Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../Engine/Module/Component/BoneSync/BoneSync.h"
#include "../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../../../GamePlay/PlayerAvatar/HitShakeReceiver/PlayerHitShakeReceiver.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../Chattable/IPlayerChattable.h"
#include "../../Input/PlayerAvatarInput_void.h"

namespace
{
    /** 火花を衝突面からどれだけ手前に置くか */
    constexpr float WALL_BLOCK_PARTICLE_SURFACE_OFFSET = 5.0f;

    /** @brief Unity の SmoothDampAngle と同形式。臨界減衰バネで current を target へ近づけた角度 [rad] を返す */
    float SmoothDampAngle(const float current, const float target, float& velocity,
                          const float smoothTime_secs, const float maxSpeed, const float deltaTime)
    {
        const float wrappedTarget = current + std::remainder(target - current, 2.0f * std::numbers::pi_v<float>);

        const float smoothTime = (std::max)(smoothTime_secs, 0.0001f);
        const float omega = 2.0f / smoothTime;
        const float x = omega * deltaTime;
        const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

        const float maxChange = maxSpeed * smoothTime;
        const float change = std::clamp(current - wrappedTarget, -maxChange, maxChange);
        const float clampedTarget = current - change;

        const float temp = (velocity + omega * change) * deltaTime;
        velocity = (velocity - omega * temp) * decay;
        float result = clampedTarget + (change + temp) * decay;

        if ((wrappedTarget - current > 0.0f) == (result > wrappedTarget))
        {
            result = wrappedTarget;
            velocity = 0.0f;
        }
        return result;
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
        footstepBoneAirborne_.clear();
        ResetAttackRotation();
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

    void SwordManAvatarStateBase::TryEmitFootstep(const std::vector<FIELD(Asset::SoundFile)>& footstepSounds)
    {
        if (!Resources().HasFootstepParticlePrefab() && footstepSounds.empty())
            return;

        const auto boneSync = Player().Components().Catch<Component::BoneSync>().lock();
        if (!boneSync)
            return;

        const auto& boneNames = Resources().FootstepBoneNames();
        if (footstepBoneAirborne_.size() != boneNames.size())
            footstepBoneAirborne_.assign(boneNames.size(), false);

        const glm::vec3 featStepPos   = FeatStepPos();
        const float     contactHeight = Resources().FootstepContactHeight();

        for (size_t boneIndex = 0; boneIndex < boneNames.size(); ++boneIndex)
        {
            const auto bonePose = boneSync->GetBoneWorldPose(boneSync->FindBoneIndex(boneNames[boneIndex]));
            if (!bonePose)
                continue;

            const float height = bonePose->Position().y - featStepPos.y;
            if (height > contactHeight)
            {
                footstepBoneAirborne_[boneIndex] = true;
                continue;
            }
            // 浮いてから降りてきた最初のフレームだけ鳴らす。接地したまま閾値付近で揺れても繰り返さない
            if (!footstepBoneAirborne_[boneIndex])
                continue;

            footstepBoneAirborne_[boneIndex] = false;

            const glm::vec3 stepPos(bonePose->Position().x, featStepPos.y, bonePose->Position().z);
            if (Resources().HasFootstepParticlePrefab())
                Scene::GameObject::Instantiate(Resources().FootstepParticlePrefab(), stepPos);

            PlayRandomSe(footstepSounds, stepPos);
        }
    }

    void SwordManAvatarStateBase::PlayRandomSe(const std::vector<FIELD(Asset::SoundFile)>& sounds, const glm::vec3& position) const
    {
        if (sounds.empty())
            return;

        static std::mt19937 seRng{ std::random_device{}() };
        std::uniform_int_distribution<size_t> pick(0, sounds.size() - 1);
        if (const auto sound = sounds[pick(seRng)].get())
            GamePlay::Sound::SoundPlayer::PlaySe(*sound, position);
    }

    void SwordManAvatarStateBase::PlayAttackSe(const bool isHit) const
    {
        const bool playsHitSound = isHit && Resources().HasAttackHitSound();
        const Asset::SoundFile& sound = playsHitSound ? Resources().AttackHitSound() : Resources().AttackWhiffSound();
        GamePlay::Sound::SoundPlayer::PlaySe(sound, Transform().GetWorldPos());
    }

    void SwordManAvatarStateBase::ResetDuringTime()
    {
        stateDuring_secs_ = 0.0f;
    }

    void SwordManAvatarStateBase::HoldHorizontalVelocity() const
    {
        RigidBody().SetLinearVelocity(glm::vec3(0.0f, RigidBody().LinearVelocity().y, 0.0f));
    }

    void SwordManAvatarStateBase::LungeForward(const float speed) const
    {
        const glm::vec3 forward = glm::normalize(glm::vec3(Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, -1.0f)));
        RigidBody().SetLinearVelocity(forward * speed + glm::vec3(0.0f, RigidBody().LinearVelocity().y, 0.0f));
    }

    void SwordManAvatarStateBase::ResetMoveSpeedFromVelocity()
    {
        const glm::vec3 velocity = RigidBody().LinearVelocity();
        currentMoveSpeed_ = glm::length(glm::vec2(velocity.x, velocity.z));
        decelerationStartSpeed_ = currentMoveSpeed_;
    }

    void SwordManAvatarStateBase::MoveForward(const StatusParameter::MoveSpeed maxSpeed, const float accelerationTime_secs, const float decelerationTime_secs)
    {
        const float targetSpeed = maxSpeed.Value();
        const float fixedDeltaTime = 1.0f / static_cast<float>(NanamiEngine::Core::Application::Configuration::PhysicsConfiguration::GetFixedUpdateRate());
        if (currentMoveSpeed_ < targetSpeed)
        {
            currentMoveSpeed_ = accelerationTime_secs <= 0.0f
                ? targetSpeed
                : (std::min)(currentMoveSpeed_ + targetSpeed / accelerationTime_secs * fixedDeltaTime, targetSpeed);
        }
        else if (currentMoveSpeed_ > targetSpeed)
        {
            // 減速レートを開始時の超過分から決め、Run→Walk のように目標が低くても decelerationTime_secs で落としきる
            currentMoveSpeed_ = decelerationTime_secs <= 0.0f
                ? targetSpeed
                : (std::max)(currentMoveSpeed_ - (decelerationStartSpeed_ - targetSpeed) / decelerationTime_secs * fixedDeltaTime, targetSpeed);
        }

        const auto inputMove = Input().Move().ReadValue();
        Actions().MoveForward(StatusParameter::MoveSpeed(currentMoveSpeed_) * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
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

    void SwordManAvatarStateBase::ShakeHitTargets(PlayerAttackArea& attackArea, const HitFeelParam& hitFeel) const
    {
        if (hitFeel.TargetShakeAmplitude() <= 0.0f || hitFeel.TargetShakeDuration_secs() <= 0.0f)
            return;

        const glm::vec3 origin = Transform().GetWorldPos();
        for (const auto& attackTarget : attackArea.Targets())
        {
            const auto shakeReceiver = attackTarget.GameObject().Components().Catch<GamePlay::PlayerAvatar::PlayerHitShakeReceiver>().lock();
            if (!shakeReceiver)
                continue;

            const glm::vec3 toTarget = attackTarget.GameObject().Transform().GetWorldPos() - origin;
            glm::vec3 direction = glm::vec3(toTarget.x, 0.0f, toTarget.z);
            direction = glm::length(direction) > 0.0001f
                ? glm::normalize(direction)
                : glm::normalize(glm::vec3(Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, -1.0f)));

            shakeReceiver->Play(direction, hitFeel.TargetShakeAmplitude(), hitFeel.TargetShakeDuration_secs());
        }
    }

    bool SwordManAvatarStateBase::TryBlockAttackByWall(PlayerAttackArea& attackArea) const
    {
        const glm::vec3 playerPos     = Transform().GetWorldPos();
        const glm::vec3 attackAreaPos = attackArea.Transform().GetWorldPos();
        // 武器が通る高さで水平に飛ばす。足元から攻撃判定の中心へ斜めに飛ばすと、
        // 目の前の壁ではなく庇のような頭上の出っ張りを拾って火花が宙に浮く
        const glm::vec3 origin(playerPos.x, attackAreaPos.y, playerPos.z);
        const glm::vec3 direction(attackAreaPos.x - playerPos.x, 0.0f, attackAreaPos.z - playerPos.z);

        const float reach = glm::length(direction);
        if (reach <= 0.0f)
            return false;

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const auto raycastHit = Physics::Raycast(origin, direction, reach, mask);
        if (!raycastHit.Hit())
            return false;

        // 壁にめり込んで隠れないよう、衝突面から自機側へ少し戻す（面の法線は裏返っていることがある）
        const glm::vec3 blockPos = raycastHit.Position() - direction / reach * WALL_BLOCK_PARTICLE_SURFACE_OFFSET;

        if (Resources().HasAttackBlockedParticlePrefab())
            Scene::GameObject::Instantiate(Resources().AttackBlockedParticlePrefab(), blockPos);

        PlayRandomSe(Resources().AttackBlockedSounds(), blockPos);

        OnChangeState(SwordManAvatarStateType::AttackedShocked);
        return true;
    }

    void SwordManAvatarStateBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
    {
        CameraGroup().ChangeCamera(camera);
    }

    void SwordManAvatarStateBase::UpdateItemPouchInput() const
    {
        auto& pouch = Status().Pouch();

        if (Input().CycleItemNext().IsPressed())
            pouch.Cycle(1);
        if (Input().CycleItemPrev().IsPressed())
            pouch.Cycle(-1);
        if (Input().UseItem().IsPressed())
            UseSelectedPouchItem();
    }

    void SwordManAvatarStateBase::UseSelectedPouchItem() const
    {
        auto& status = Status();
        const auto* selected = status.Pouch().Selected();
        if (selected == nullptr || selected->count <= 0 || !selected->item)
            return;

        const auto& item = *selected->item;
        switch (item.Effect())
        {
        case Asset::ItemEffectType::HealHealth:
            status.Heal(StatusParameter::Health(static_cast<int>(item.EffectAmount())));
            break;
        case Asset::ItemEffectType::RestoreStamina:
            status.RestoreStamina(item.EffectAmount());
            break;
        case Asset::ItemEffectType::EnhanceAttack:
            status.ApplyAttackBuff(item.EffectAmount(), item.EffectDuration_secs());
            break;
        // 効果がまだ無いアイテム(罠や爆弾)は減らさない
        case Asset::ItemEffectType::None:
            return;
        }

        status.Pouch().ConsumeSelected();
        if (const auto sound = item.UseSound())
            GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
    }

    Damage::PhysicsPower SwordManAvatarStateBase::BuffedAttackPower(const Damage::PhysicsPower base) const
    {
        const float rate = Status().AttackPowerRate();
        if (rate == 1.0f)
            return base;
        return Damage::PhysicsPower(static_cast<int>(static_cast<float>(base.Value()) * rate));
    }

    void SwordManAvatarStateBase::UpdateLockOn() const
    {
        if (CameraGroup().IsLockedOn() && !IsLockOnTargetInRange())
            CameraGroup().ReleaseLockOn();
        
        const auto nearestTarget = CameraGroup().IsLockedOn() ? nullptr : FindNearestLockOnTarget();
        CameraGroup().SetLockOnCandidate(nearestTarget);

        if (!Input().LockOn().IsPressed())
            return;

        if (CameraGroup().IsLockedOn())
        {
            CameraGroup().ReleaseLockOn();
            return;
        }

        if (nearestTarget)
        {
            CameraGroup().EngageLockOn(nearestTarget);
            StatusEvent().InvokeOnLockOn();
        }
    }

    void SwordManAvatarStateBase::VisitLockOnAction(ISwordManAvatarTransitionVisitor& visitor) const
    {
        if (ExpiredCamera())
            return;

        const bool isLockedOn = CameraGroup().IsLockedOn();
        visitor.Action(
            isLockedOn ? SwordManAvatarStateAction::LockOnRelease : SwordManAvatarStateAction::LockOn,
            isLockedOn || !CameraGroup().LockOnCandidate().expired());
    }

    namespace
    {
        class TransitionExecutor final : public ISwordManAvatarTransitionVisitor
        {
        public:
            TransitionExecutor(
                const SwordManAvatarInputAction& input,
                const std::function<void(SwordManAvatarStateType)>& onChangeState)
                : input_(input)
                , onChangeState_(onChangeState)
            {
            }

            bool Automatic(const SwordManAvatarStateType to, const bool condition) override
            {
                return TryChange(to, condition);
            }

            bool OnInput(const SwordManAvatarStateType to, const SwordManAvatarInput input, const SwordManAvatarInputPhase phase, const bool isUsable) override
            {
                return TryChange(to, isUsable && IsTriggered(input, phase));
            }

            bool OnInputWhenReady(const SwordManAvatarStateType to, const SwordManAvatarInput input, const SwordManAvatarInputPhase phase, const bool isUsable, const bool isReady) override
            {
                return TryChange(to, isUsable && isReady && IsTriggered(input, phase));
            }

            void Action(SwordManAvatarStateAction, bool) override {}

            [[nodiscard]] bool HasChanged() const { return hasChanged_; }

        private:
            bool TryChange(const SwordManAvatarStateType to, const bool condition)
            {
                if (!condition)
                    return false;

                onChangeState_(to);
                hasChanged_ = true;
                return true;
            }

            template <typename T>
            static bool IsInPhase(const PlayerAvatarInput<T>& input, const SwordManAvatarInputPhase phase)
            {
                switch (phase)
                {
                case SwordManAvatarInputPhase::Pressed:    return input.IsPressed();
                case SwordManAvatarInputPhase::Holding:    return input.IsUpdatePressed();
                case SwordManAvatarInputPhase::NotHolding: return !input.IsUpdatePressed();
                }
                return false;
            }

            [[nodiscard]] bool IsTriggered(const SwordManAvatarInput input, const SwordManAvatarInputPhase phase) const
            {
                switch (input)
                {
                case SwordManAvatarInput::Move:         return IsInPhase(input_.Move(),         phase);
                case SwordManAvatarInput::Run:          return IsInPhase(input_.Run(),          phase);
                case SwordManAvatarInput::Jump:         return IsInPhase(input_.Jump(),         phase);
                case SwordManAvatarInput::AvoidRolling: return IsInPhase(input_.AvoidRolling(), phase);
                case SwordManAvatarInput::NormalAttack: return IsInPhase(input_.NormalAttack(), phase);
                case SwordManAvatarInput::DashAttack:   return IsInPhase(input_.DashAttack(),   phase);
                case SwordManAvatarInput::CannonAttack: return IsInPhase(input_.CannonAttack(), phase);
                case SwordManAvatarInput::Chat:         return IsInPhase(input_.Chat(),         phase);
                case SwordManAvatarInput::LockOn:       return IsInPhase(input_.LockOn(),       phase);
                case SwordManAvatarInput::CycleItemNext:return IsInPhase(input_.CycleItemNext(),phase);
                case SwordManAvatarInput::CycleItemPrev:return IsInPhase(input_.CycleItemPrev(),phase);
                case SwordManAvatarInput::UseItem:      return IsInPhase(input_.UseItem(),      phase);
                }
                return false;
            }

            const SwordManAvatarInputAction& input_;
            const std::function<void(SwordManAvatarStateType)>& onChangeState_;
            bool hasChanged_ = false;
        };
    }

    bool SwordManAvatarStateBase::UpdateTransitions() const
    {
        TransitionExecutor executor(Input(), onChangeState_);
        VisitTransitions(executor);
        return executor.HasChanged();
    }

    void SwordManAvatarStateBase::RotateTowardsAttackTarget(const float smoothTime_secs, const float maxRotateSpeed)
    {
        const auto target = ResolveAttackTarget();
        if (!target)
        {
            attackYawVelocity_ = 0.0f;
            return;
        }

        const glm::vec3 toTarget = target->Transform().GetWorldPos() - Transform().GetWorldPos();
        if (toTarget.x * toTarget.x + toTarget.z * toTarget.z < 0.0001f)
            return;

        // 前方は -Z
        const glm::quat currentRot = Transform().GetWorldRot();
        const glm::vec3 forward = currentRot * glm::vec3(0.0f, 0.0f, -1.0f);
        const float currentYaw = std::atan2(-forward.x, -forward.z);
        const float targetYaw  = std::atan2(-toTarget.x, -toTarget.z);
        const float newYaw = SmoothDampAngle(currentYaw, targetYaw, attackYawVelocity_, smoothTime_secs, maxRotateSpeed, Time::DeltaTime());

        Transform().SetWorldRot(glm::angleAxis(newYaw - currentYaw, glm::vec3(0.0f, 1.0f, 0.0f)) * currentRot);
    }

    void SwordManAvatarStateBase::ResetAttackRotation()
    {
        attackAutoAimTarget_.reset();
        attackYawVelocity_ = 0.0f;
    }

    std::shared_ptr<GameObject::IGameObject> SwordManAvatarStateBase::ResolveAttackTarget()
    {
        if (ExpiredCamera())
            return nullptr;

        if (CameraGroup().IsLockedOn())
            return CameraGroup().LockOnTarget().lock();

        auto target = attackAutoAimTarget_.lock();
        if (!target)
        {
            target = FindNearestLockOnTarget();
            attackAutoAimTarget_ = target;
        }
        return target;
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
        
        const auto targetCollider = target->Components().Catch<Physics::ICollider>().lock();
        const glm::vec3 targetPos = targetCollider
            ? targetCollider->CenterOfMassPosition().value_or(target->Transform().GetWorldPos())
            : target->Transform().GetWorldPos();
        const glm::vec3 diff = targetPos - origin;
        const float distance = glm::length(diff);
        if (distance <= 0.0f)
            return true;
        
        // 敵は遮蔽物に含めない
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const auto hit = Physics::Raycast(origin, diff, distance, mask);
        return !hit.Hit() || &hit.HitObject() == target.get();
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
