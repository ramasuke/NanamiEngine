#pragma once
#include <string>
#include <vector>

#include "SwordManAvatarStateType.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../State/PlayerAvatarStateBase.h"
#include "../Animation/SwordManAvatarAnimation.h"
#include "../InputAction/SwordManAvatarInputAction.h"
#include "../Status/SwordManAvatarStatus.h"
#include "Context/SwordManAvatarStateContext.h"
#include "Transition/SwordManAvatarStateTransition.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    using SwordManAvatarStateArgs = PlayerAvatarStateArgs<SwordManAvatarStateContext, SwordManAvatarStateType>;

    class SwordManAvatarStateBase : public PlayerAvatarStateBase<SwordManAvatarStateContext,
                                                                 SwordManAvatarStateType,
                                                                 SwordMan::AnimationType,
                                                                 ISwordManAvatarTransitionVisitor>
    {
    public:
        explicit SwordManAvatarStateBase(const SwordManAvatarStateArgs& args);

        virtual ~SwordManAvatarStateBase() override = default;
        [[nodiscard]] virtual bool MouseLock() { return true; }

    protected:
        struct MoveSpeedRamp
        {
            float current           = 0.0f;
            float decelerationStart = 0.0f;
        };
        struct FootstepLatch
        {
            std::vector<bool> boneAirborne;
        };
        struct AttackTurn
        {
            std::weak_ptr<GameObject::IGameObject> autoAimTarget;
            float yawVelocity = 0.0f;
        };

    private:
        void OnLockOnEngaged() const override;
        void UseSelectedPouchItem() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestLockOnTarget() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> ResolveAttackTarget(AttackTurn& turn) const;

    protected:
        /** ---- 以下サンドボックスパターン ---- */
        [[nodiscard]] State::IStatusEventSubject&            StatusEvent     () const { return Status().Subject                 (); }
        [[nodiscard]] GamePlay::Ui::NpcChatting &            NpcChattingUi   () const { return Context().NpcChattingUi          (); }
        [[nodiscard]] glm::vec3                              FeatStepPos     () const { return Context().PlayerAvatarFeatStepPos(); }
        [[nodiscard]] GamePlay::PlayerAvatar::WakeUpArea   & WakeUpArea      () const { return Context().WakeUpArea             (); }
        [[nodiscard]] PlayerAttackArea& NormalAttackArea   () const { return Context().NormalAttackArea   (); }
        [[nodiscard]] PlayerAttackArea& DashAttackArea     () const { return Context().DashAttackArea     (); }
        [[nodiscard]] GamePlay::PlayerAvatar::LockOnDetectionArea& LockOnDetectionArea() const { return Context().LockOnDetectionArea(); }
        [[nodiscard]] Component::ParticleSystem& SuccessAvoidRollingParticle() const { return Context().SuccessAvoidRollingParticle(); }

        void TryEmitFootstep(FootstepLatch& latch, const std::vector<FIELD(Asset::SoundFile)>& footstepSounds) const;
        void PlayRandomSe(const std::vector<FIELD(Asset::SoundFile)>& sounds, const glm::vec3& position) const;
        void PlayAttackSe(bool isHit) const;
        void ResetMoveSpeedFromVelocity(MoveSpeedRamp& ramp) const;
        void LungeForward(float speed) const;
        void MoveForward(MoveSpeedRamp& ramp, StatusParameter::MoveSpeed maxSpeed, float accelerationTime_secs, float decelerationTime_secs) const;
        // VisitTransitions で CycleItem / UseItem を宣言したStateだけが呼ぶ（アイテム欄の表示がその宣言を見ている）
        void UpdateItemPouchInput() const;
        [[nodiscard]] Damage::PhysicsPower BuffedAttackPower(Damage::PhysicsPower base) const;
        bool UpdateTransitions() const;
        void RotateTowardsAttackTarget(AttackTurn& turn, float smoothTime_secs, float maxRotateSpeed) const;
        void DealDamageText(PlayerAttackArea& attackArea, Damage::PhysicsPower power, bool isChargedAttack) const;
        void ShakeHitTargets(PlayerAttackArea& attackArea, const HitFeelParam& hitFeel) const;
        // 弾かれたら AttackedShocked へ遷移する
        bool TryBlockAttackByWall(PlayerAttackArea& attackArea) const;
    };
}
