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
#include "../../LockOnTarget/ILockOnTarget.h"
#include "../../LockOnTarget/PlayerAvatarLockOn.h"

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
    SwordManAvatarStateBase::SwordManAvatarStateBase(const SwordManAvatarStateArgs& args)
        : PlayerAvatarStateBase(args)
    {
    }

    void SwordManAvatarStateBase::TryEmitFootstep(FootstepLatch& latch, const std::vector<FIELD(Asset::SoundFile)>& footstepSounds) const
    {
        if (!Resources().HasFootstepParticlePrefab() && footstepSounds.empty())
            return;

        const auto boneSync = Player().Components().Catch<Component::BoneSync>().lock();
        if (!boneSync)
            return;

        const auto& boneNames = Resources().FootstepBoneNames();
        if (latch.boneAirborne.size() != boneNames.size())
            latch.boneAirborne.assign(boneNames.size(), false);

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
                latch.boneAirborne[boneIndex] = true;
                continue;
            }
            // 浮いてから降りてきた最初のフレームだけ鳴らす。接地したまま閾値付近で揺れても繰り返さない
            if (!latch.boneAirborne[boneIndex])
                continue;

            latch.boneAirborne[boneIndex] = false;

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

    void SwordManAvatarStateBase::LungeForward(const float speed) const
    {
        const glm::vec3 forward = glm::normalize(glm::vec3(Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, -1.0f)));
        const glm::vec3 lunge = Actions().LimitToWalkableSlope(glm::vec3(forward.x, 0.0f, forward.z) * speed);
        RigidBody().SetLinearVelocity(lunge + glm::vec3(0.0f, RigidBody().LinearVelocity().y, 0.0f));
    }

    void SwordManAvatarStateBase::ResetMoveSpeedFromVelocity(MoveSpeedRamp& ramp) const
    {
        const glm::vec3 velocity = RigidBody().LinearVelocity();
        ramp.current           = glm::length(glm::vec2(velocity.x, velocity.z));
        ramp.decelerationStart = ramp.current;
    }

    void SwordManAvatarStateBase::MoveForward(MoveSpeedRamp& ramp, const StatusParameter::MoveSpeed maxSpeed, const float accelerationTime_secs, const float decelerationTime_secs) const
    {
        const float targetSpeed = maxSpeed.Value();
        const float fixedDeltaTime = 1.0f / static_cast<float>(NanamiEngine::Core::Application::Configuration::PhysicsConfiguration::GetFixedUpdateRate());
        if (ramp.current < targetSpeed)
        {
            ramp.current = accelerationTime_secs <= 0.0f
                ? targetSpeed
                : (std::min)(ramp.current + targetSpeed / accelerationTime_secs * fixedDeltaTime, targetSpeed);
        }
        else if (ramp.current > targetSpeed)
        {
            // 減速レートを開始時の超過分から決め、Run→Walk のように目標が低くても decelerationTime_secs で落としきる
            ramp.current = decelerationTime_secs <= 0.0f
                ? targetSpeed
                : (std::max)(ramp.current - (ramp.decelerationStart - targetSpeed) / decelerationTime_secs * fixedDeltaTime, targetSpeed);
        }

        const auto inputMove = Input().Move().ReadValue();
        Actions().MoveForward(StatusParameter::MoveSpeed(ramp.current) * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
    }

    void SwordManAvatarStateBase::DealDamageText(PlayerAttackArea& attackArea, const Damage::PhysicsPower power, const bool isChargedAttack) const
    {
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const glm::vec3 attackPosition = attackArea.Transform().GetWorldPos();

        for (const auto& attackTarget : attackArea.Targets())
        {
            // ダメージ側(AttackArea::ApplyPhysicsAttack)と同じ基準で当たった部位を決める
            const auto hitPart = attackTarget.NearestPart(attackPosition).lock();

            const auto origin    = Transform().GetWorldPos();
            const auto targetPos = attackTarget.GameObject().Transform().GetWorldPos();
            const auto direction = targetPos - origin;

            const auto raycastHit = Physics::Raycast(
                                            origin,
                                            direction,
                                            glm::length(direction),
                                            mask);

            const auto textPos = raycastHit.Hit() ? raycastHit.Position() : targetPos;
            GamePlay::Ui::SpawnDealDamageText(Resources().DealDamageTextBillBoardPrefab(), textPos, power.Value(),
                                              hitPart, attackTarget.GameObject(), isChargedAttack);
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
        const auto used = Status().Pouch().UseSelected(Status());
        if (!used)
            return;

        if (const auto sound = used->UseSound())
            GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
    }

    Damage::PhysicsPower SwordManAvatarStateBase::BuffedAttackPower(const Damage::PhysicsPower base) const
    {
        const float rate = Status().AttackPowerRate();
        if (rate == 1.0f)
            return base;
        return Damage::PhysicsPower(static_cast<int>(static_cast<float>(base.Value()) * rate));
    }

    void SwordManAvatarStateBase::OnLockOnEngaged() const
    {
        StatusEvent().InvokeOnLockOn();
    }

    namespace
    {
        class SwordManTransitionExecutor final : public PlayerAvatarTransitionExecutorBase<ISwordManAvatarTransitionVisitor>
        {
        public:
            SwordManTransitionExecutor(
                const SwordManAvatarInputAction& input,
                const std::function<void(SwordManAvatarStateType)>& onChangeState)
                : PlayerAvatarTransitionExecutorBase(onChangeState)
                , input_(input)
            {
            }

            bool OnInputWhenReady(const SwordManAvatarStateType to, const SwordManAvatarInput input, const PlayerAvatarInputPhase phase, const bool isUsable, const bool isReady) override
            {
                return TryChange(to, isUsable && isReady && IsTriggered(input, phase));
            }

        private:
            [[nodiscard]] bool IsTriggered(const SwordManAvatarInput input, const PlayerAvatarInputPhase phase) const override
            {
                switch (input)
                {
                case SwordManAvatarInput::Move:         return IsInputInPhase(input_.Move(),         phase);
                case SwordManAvatarInput::Run:          return IsInputInPhase(input_.Run(),          phase);
                case SwordManAvatarInput::Jump:         return IsInputInPhase(input_.Jump(),         phase);
                case SwordManAvatarInput::AvoidRolling: return IsInputInPhase(input_.AvoidRolling(), phase);
                case SwordManAvatarInput::NormalAttack: return IsInputInPhase(input_.NormalAttack(), phase);
                case SwordManAvatarInput::DashAttack:   return IsInputInPhase(input_.DashAttack(),   phase);
                case SwordManAvatarInput::CannonAttack: return IsInputInPhase(input_.CannonAttack(), phase);
                case SwordManAvatarInput::Chat:         return IsInputInPhase(input_.Chat(),         phase);
                case SwordManAvatarInput::LockOn:       return IsInputInPhase(input_.LockOn(),       phase);
                case SwordManAvatarInput::CycleItemNext:return IsInputInPhase(input_.CycleItemNext(),phase);
                case SwordManAvatarInput::CycleItemPrev:return IsInputInPhase(input_.CycleItemPrev(),phase);
                case SwordManAvatarInput::UseItem:      return IsInputInPhase(input_.UseItem(),      phase);
                }
                return false;
            }

            const SwordManAvatarInputAction& input_;
        };
    }

    bool SwordManAvatarStateBase::UpdateTransitions() const
    {
        SwordManTransitionExecutor executor(Input(), OnChangeStateCallback());
        VisitTransitions(executor);
        return executor.HasChanged();
    }

    void SwordManAvatarStateBase::RotateTowardsAttackTarget(AttackTurn& turn, const float smoothTime_secs, const float maxRotateSpeed) const
    {
        const auto target = ResolveAttackTarget(turn);
        if (!target)
        {
            turn.yawVelocity = 0.0f;
            return;
        }

        // 部位グループの Transform は本体の原点にあるので、狙う点へ向く
        const glm::vec3 toTarget = LockOnPositionOf(*target) - Transform().GetWorldPos();
        if (toTarget.x * toTarget.x + toTarget.z * toTarget.z < 0.0001f)
            return;

        // 前方は -Z
        const glm::quat currentRot = Transform().GetWorldRot();
        const glm::vec3 forward = currentRot * glm::vec3(0.0f, 0.0f, -1.0f);
        const float currentYaw = std::atan2(-forward.x, -forward.z);
        const float targetYaw  = std::atan2(-toTarget.x, -toTarget.z);
        const float newYaw = SmoothDampAngle(currentYaw, targetYaw, turn.yawVelocity, smoothTime_secs, maxRotateSpeed, Time::DeltaTime());

        Transform().SetWorldRot(glm::angleAxis(newYaw - currentYaw, glm::vec3(0.0f, 1.0f, 0.0f)) * currentRot);
    }

    std::shared_ptr<GameObject::IGameObject> SwordManAvatarStateBase::ResolveAttackTarget(AttackTurn& turn) const
    {
        if (ExpiredCamera())
            return nullptr;

        if (CameraGroup().IsLockedOn())
            return CameraGroup().LockOnAim();

        auto target = turn.autoAimTarget.lock();
        if (!target)
        {
            target = FindNearestLockOnTarget();
            turn.autoAimTarget = target;
        }
        return target;
    }

    std::shared_ptr<GameObject::IGameObject> SwordManAvatarStateBase::FindNearestLockOnTarget() const
    {
        return GameCore::PlayerAvatar::LockOn::FindNearestTarget(LockOnDetectionArea(), Transform().GetWorldPos());
    }
}
