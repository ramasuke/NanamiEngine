#pragma once
#include <optional>
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

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

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
            struct Bone
            {
                bool                 armed = false;
                std::optional<float> prevHeight;
            };
            std::vector<Bone> bones;
        };
        struct AttackTurn
        {
            std::weak_ptr<GameObject::IGameObject> autoAimTarget;
            float yawVelocity = 0.0f;
        };

    private:
        void OnLockOnEngaged() const override;
        /** @return 使うモーションのステートへ移ったら true */
        bool UseSelectedPouchItem() const;
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
        /** 専用の空振り/ヒット音を鳴らす。未設定の側は PlayAttackSe(isHit) と同じ共通の音になる */
        void PlayAttackSe(bool isHit, const FIELD(Asset::SoundFile)& whiffSound, const FIELD(Asset::SoundFile)& hitSound) const;
        void ResetMoveSpeedFromVelocity(MoveSpeedRamp& ramp) const;
        void LungeForward(float speed) const;
        /** @return 再生中の移動系クリップ(Idle/Walk/Run...)のブレンド率の合計 [0,1]。AnimationTree が無ければ 1 */
        [[nodiscard]] float LocomotionBlendRate() const;
        void MoveForward(MoveSpeedRamp& ramp, StatusParameter::MoveSpeed maxSpeed, float accelerationTime_secs, float decelerationTime_secs) const;
        bool UpdateItemPouchInput() const;
        [[nodiscard]] Damage::PhysicsPower BuffedAttackPower(Damage::PhysicsPower base) const;
        bool UpdateTransitions() const;
        // 攻撃ボタンの押下。溜められるなら構えて待ち、離すと通常攻撃・押し続けると溜めになる
        void VisitNormalAttackPress(ISwordManAvatarTransitionVisitor& visitor) const;
        void RotateTowardsAttackTarget(AttackTurn& turn, float smoothTime_secs, float maxRotateSpeed) const;
        void DealDamageText(PlayerAttackArea& attackArea, Damage::PhysicsPower power) const;
        /** 攻撃の演出を出し、オンラインなら他のピアにも出させる。rotation / scale が無ければプレハブのまま */
        void SpawnAttackParticle(Asset::PrefabGameObjectFile& prefab, const glm::vec3& position,
                                 const std::optional<glm::quat>& rotation = std::nullopt, std::optional<float> scale = std::nullopt) const;
        void ShakeHitTargets(PlayerAttackArea& attackArea, const HitFeelParam& hitFeel) const;
        // 弾かれたら AttackedShocked へ遷移する
        bool TryBlockAttackByWall(PlayerAttackArea& attackArea) const;
    };
}
