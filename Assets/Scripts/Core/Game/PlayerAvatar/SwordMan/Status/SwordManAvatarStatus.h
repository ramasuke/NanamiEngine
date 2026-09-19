#pragma once
#include "../../../StatusParameter/Health/Health.h"
#include "../../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../StatusParameter/Stamina/Stamina.h"
#include "../../Status/IPlayerAvatarStatus.h"
#include "../../Status/BasicParams/AttackParam/AttackParam.h"
#include "../../Status/BasicParams/HitFeelParam/HitFeelParam.h"
#include "../../Status/EnahancePower/EnhancePower.h"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include <queue>

#include "../../../../../../../Engine/Core/Network/Object/NetworkObjectBase.h"
#include "../../../../../../../Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../StateMachine/IReadOnlyPlayerAvatarStateMachine.h"
#include "../State/SwordManAvatarStateType.h"
#include "ControlGuideFocus/SwordMan_ControlGuideFocus.h"
#include "Event/SwordManAvatarStatusEvent.h"
#include "Quest/SwordMan_QuestGroup.h"
#include "../../Item/ItemPouch.h"
#include "../../Item/Effect/IItemEffectTarget.h"
#include "../../Item/IItemReceiver.h"
#include "../../Wallet/PlayerAvatar_Wallet.h"

namespace NanamiEngine::Module::Asset
{
    class SwordManInitStatus;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus final : public NetworkObjectBase,
                                       public IPlayerAvatarStatus,
                                       public Item::IItemEffectTarget,
                                       public Item::IItemReceiver
    {
    public:
        SwordManAvatarStatus();
        explicit SwordManAvatarStatus(const Asset::SwordManInitStatus& initStatus);
        ~SwordManAvatarStatus() override;
        SwordManAvatarStatus(SwordManAvatarStatus&&) noexcept = default;
        SwordManAvatarStatus& operator=(SwordManAvatarStatus&&) noexcept = default;
        void Init    () override;
        void OnUpdate() override;
        
        [[nodiscard]] IObservableStatusEvent    & Observable    () const          { return *event_ ; }
        [[nodiscard]] State::IStatusEventSubject& Subject       () const          { return *event_ ; }
        [[nodiscard]] IStatusEvent              & Event         () const override { return *event_ ; }
        [[nodiscard]] QuestGroup                & Quest         () const override { return *quests_; }
        [[nodiscard]] Quest::ICompleteQuestGroup& CompletedQuest() const override { return *quests_; }
        [[nodiscard]] PlayerAvatar::Wallet      & Wallet        () const override { return *wallet_; }
        [[nodiscard]] IControlGuideFocusRequest     & GuideFocusRequest     () const { return *controlGuideFocus_; }
        [[nodiscard]] IControlGuideFocusPresentation& GuideFocusPresentation() const { return *controlGuideFocus_; }
        
        [[nodiscard]] const StatusParameter::Health&                                MaxHealth() const override { return maxHealth_;           }
        [[nodiscard]] rxcpp::observable<StatusParameter::Health>         OnChangeHealth() const override { return onChangeHealth_.get_observable(); }
        [[nodiscard]] StatusParameter::Health                            Health() const override { return currentHealth_->Get(); }
        [[nodiscard]] bool                                               IsDeath  () const override { return minHealth_ >= currentHealth_->Get();   }
        [[nodiscard]] bool                                               IsInjured() const override;
        [[nodiscard]] bool                                               IsDowned () const override { return isDowned_; }
                      void                                               SetDowned(bool downed) { isDowned_ = downed; }
                      void                                               Revive() override;
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit>               OnBecomeInjured    () const override { return onBecomeInjured_    .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit>               OnRecoverFromInjured() const override { return onRecoverFromInjured_.get_observable(); }
        [[nodiscard]] const StatusParameter::Stamina&                                MaxStamina() const override { return maxStamina_;           }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Stamina> Stamina   () const override { return stamina_.AsReadOnly(); }
        [[nodiscard]] bool                                                           CanRun    () const override { return !isStaminaExhausted_; }
        [[nodiscard]] bool                                                           CanAvoidRolling() const { return stamina_.get() >= StatusParameter::Stamina(avoidRollingStaminaCost_); }
        [[nodiscard]] bool                                                           CanChargeAttack() const { return stamina_.get() >= StatusParameter::Stamina(chargeAttackStaminaCost_); }
                      void                                                           SetStateMachine(const IReadOnlyPlayerAvatarStateMachine<SwordManAvatarStateType>& stateMachine) { stateMachine_ = &stateMachine; }

        [[nodiscard]] const std::vector<AttackParam<Damage::PhysicsPower>>& ComboNormalAttack() const { return comboNormalAttack_; }
        [[nodiscard]] float                             ComboNormalAttackStateDuration_secs  () const { return comboNormalAttackStateDuration_secs_; }
        [[nodiscard]] float                             AttackedShockedStateDuration_secs    () const { return attackedShockedStateDuration_secs_; }
        [[nodiscard]] const std::vector<HitFeelParam>&  ComboHitFeel                         () const { return comboHitFeel_; }
        /** @brief comboIndex 段目の踏み込み速度。用意されていない段は踏み込まない */
        [[nodiscard]] float                             ComboAttackLungeSpeed                (const int comboIndex) const
        {
            return comboIndex < static_cast<int>(comboHitFeel_.size()) ? comboHitFeel_[comboIndex].LungeSpeed() : 0.0f;
        }
        [[nodiscard]] const HitFeelParam&               DashHitFeel                          () const { return dashHitFeel_; }
        [[nodiscard]] float                             ComboInputBufferWindow_secs          () const { return comboInputBufferWindow_secs_; }
        [[nodiscard]] float                             ChargeAttackHoldThreshold_secs       () const { return chargeAttackHoldThreshold_secs_; }
        [[nodiscard]] float                             ChargeAttackMaxCharge_secs           () const { return chargeAttackMaxCharge_secs_; }
        [[nodiscard]] float                             ChargeAttackMaxHold_secs             () const { return chargeAttackMaxHold_secs_; }
        [[nodiscard]] const AttackParam<Damage::PhysicsPower>& ChargeAttack                  () const { return chargeAttack_; }
        [[nodiscard]] const HitFeelParam&               ChargeHitFeel                        () const { return chargeHitFeel_; }
        [[nodiscard]] float                             ChargeAttackLungeStart_secs          () const { return chargeAttackLungeStart_secs_; }
        [[nodiscard]] float                             ChargeAttackLungeSpeed               () const { return chargeAttackLungeSpeed_; }
        [[nodiscard]] float                             ChargeAttackStaminaCost              () const { return chargeAttackStaminaCost_; }
        [[nodiscard]] const AttackParam<Damage::PhysicsPower>& JumpAttack                    () const { return jumpAttack_; }
        [[nodiscard]] const HitFeelParam&               JumpAttackHitFeel                    () const { return jumpAttackHitFeel_; }
        [[nodiscard]] float                             JumpAttackWindup_secs                () const { return jumpAttackWindup_secs_; }
        [[nodiscard]] float                             JumpAttackPlungeSpeed                () const { return jumpAttackPlungeSpeed_; }
        [[nodiscard]] StatusParameter::MoveSpeed        GetWalkSpeed                        () const override { return walkSpeed_;                }
        [[nodiscard]] StatusParameter::MoveSpeed        GetRunSpeed                          () const override { return runSpeed_ ;                }
        [[nodiscard]] float                             GetMoveRotateSpeed                   () const override { return moveRotateSpeed_;          }
        [[nodiscard]] float                             LockOnAttackRotateSpeed              () const          { return lockOnAttackRotateSpeed_;  }
        [[nodiscard]] float                             AttackRotateSmoothTime_secs          () const          { return attackRotateSmoothTime_secs_; }
        [[nodiscard]] float                             GetJumpPower                         () const override { return jumpPower_;                }
        [[nodiscard]] float                             GetJumpStateDuration_secs            () const override { return jumpStateDuration_secs_;   }
        [[nodiscard]] float                             JumpCooldown_secs                    () const          { return jumpCooldown_secs_;        }
        [[nodiscard]] bool                              CanJump                              () const          { return jumpCooldownRemaining_secs_ <= 0.0f && stamina_.get() >= StatusParameter::Stamina(jumpStaminaCost_); }
        [[nodiscard]] AttackParam<Damage::PhysicsPower> DashAttack                           () const          { return dashAttack_;  }
        [[nodiscard]] float                             DashAttackLungeSpeed                 () const          { return dashAttackLungeSpeed_secs_; }
        [[nodiscard]] bool                              IsDamaged                            () const;
        [[nodiscard]] float                             DamageStateDuration_secs             () const   { return damageStateDuration_secs_; }
        [[nodiscard]] float                             AvoidRollingStateDuration_secs       () const   { return avoidRollingStateDuration_secs_; }
        [[nodiscard]] float                             AvoidRollingStaminaCost              () const   { return avoidRollingStaminaCost_; }
        [[nodiscard]] float                             DeathStateDuration_secs              () const   { return deathStateDuration_secs_; }
        [[nodiscard]] float                             DownStateDuration_secs               () const   { return downStateDuration_secs_; }
        [[nodiscard]] float                             FallDownStateDuration_secs           () const   { return fallDownStateDuration_secs_; }
        [[nodiscard]] float                             GetUpStateDuration_secs              () const   { return getUpStateDuration_secs_; }
                      void                              AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;
                      void                              ApplyDamage();
                      void                              DiscardDamage();
                      void                              ConsumeAvoidRollingStamina();
                      void                              ConsumeChargeAttackStamina();
                      void                              ConsumeJumpStamina();
                      void                              StartJumpCooldown();
        /** @brief 体力を amount だけ戻す。最大値で頭打ち、死亡中は何もしない */
                      void                              Heal(StatusParameter::Health amount) override;
                      void                              RestoreStamina(float amount) override;
        /** @brief 攻撃力の倍率を duration_secs のあいだ差し替える。重ねがけは上書き */
                      void                              ApplyAttackBuff(float rate, float duration_secs) override;
        [[nodiscard]] float                             AttackPowerRate() const { return attackBuffRemaining_secs_ > 0.0f ? attackBuffRate_ : 1.0f; }
        [[nodiscard]] float                             AttackBuffRemaining_secs() const { return attackBuffRemaining_secs_; }
        [[nodiscard]] ItemPouch&                        Pouch()       override { return pouch_; }
        [[nodiscard]] const ItemPouch&                  Pouch() const override { return pouch_; }
                      void                              SetupPouch(const std::vector<Asset::ItemStack>& initialItems) { pouch_.Setup(initialItems); }
        [[nodiscard]] int                               ReceivableCount(const Asset::ItemData& item) const override { return pouch_.ReceivableCount(item); }
                      int                               ReceiveItem(const std::shared_ptr<Asset::ItemData>& item, const int count) override { return pouch_.Add(item, count); }
        
        
    private:
        std::shared_ptr<StatusEvent> event_;
        std::shared_ptr<ControlGuideFocus> controlGuideFocus_ = std::make_shared<ControlGuideFocus>();
        [[serialize(0)]] std::unique_ptr<QuestGroup> quests_;
        [[serialize(20)]] std::shared_ptr<PlayerAvatar::Wallet> wallet_;
        
        [[serialize(0)]] StatusParameter::Health maxHealth_;
        [[serialize(0)]] StatusParameter::Health minHealth_;
        rxcpp::subjects::subject<StatusParameter::Health> onChangeHealth_;
        [[serialize(0)]] SyncParam<StatusParameter::Health> currentHealth_ = SyncParamFactory::Create<StatusParameter::Health>(this, StatusParameter::Health(100));
        
        [[serialize(0)]] StatusParameter::Stamina maxStamina_;
        [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Stamina> stamina_;
        [[serialize(0)]] float staminaDrainPerSecond_;
        [[serialize(0)]] float staminaRegenPerSecond_;
        [[serialize(0)]] float minStaminaRatioToResumeRun_ = 0.3f;
        bool isStaminaExhausted_ = false;
        const IReadOnlyPlayerAvatarStateMachine<SwordManAvatarStateType>* stateMachine_ = nullptr;

        [[serialize(0)]] std::vector<AttackParam<Damage::PhysicsPower>> comboNormalAttack_;
        [[serialize(0)]] float comboNormalAttackStateDuration_secs_;
        [[serialize(0)]] float attackedShockedStateDuration_secs_;
        [[serialize(0)]] AttackParam<Damage::PhysicsPower> dashAttack_;
        [[serialize(8)]] float dashAttackLungeSpeed_secs_; 
        [[serialize(9)]] std::vector<HitFeelParam> comboHitFeel_;
        [[serialize(9)]] HitFeelParam dashHitFeel_; 
        [[serialize(9)]] float comboInputBufferWindow_secs_;
        [[serialize(11)]] float chargeAttackHoldThreshold_secs_; ///< NormalAttack開始からこの時間押し続けたら溜めへ移行(1段目の発生より短くする)
        [[serialize(11)]] float chargeAttackMaxCharge_secs_;     ///< 溜め開始から最大溜めに達するまでの時間。これ未満で離すと通常コンボ
        [[serialize(11)]] float chargeAttackMaxHold_secs_;       ///< 最大溜めのまま保持できる上限。超えると自動解放
        [[serialize(11)]] AttackParam<Damage::PhysicsPower> chargeAttack_;
        [[serialize(11)]] HitFeelParam chargeHitFeel_;
        [[serialize(11)]] float chargeAttackLungeStart_secs_; ///< 解放ステート開始から前方への踏み込みを始める時間。発生時に止める
        [[serialize(11)]] float chargeAttackLungeSpeed_;
        [[serialize(11)]] float chargeAttackStaminaCost_;
        [[serialize(17)]] AttackParam<Damage::PhysicsPower> jumpAttack_; ///< 着地の叩きつけ。発生・持続は JumpAttackLand ステート開始から
        [[serialize(17)]] HitFeelParam jumpAttackHitFeel_;
        [[serialize(17)]] float jumpAttackWindup_secs_; ///< 空中で振りかぶって止まる時間。過ぎたら真下へ急降下する
        [[serialize(17)]] float jumpAttackPlungeSpeed_;

        [[serialize(0)]] StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] StatusParameter::MoveSpeed runSpeed_ ;
        [[serialize(0)]] float                      moveRotateSpeed_;
        [[serialize(7)]] float                      lockOnAttackRotateSpeed_;
        [[serialize(15)]] float                     attackRotateSmoothTime_secs_;
        [[serialize(0)]]  float                     jumpPower_;
        [[serialize(10)]] float                     jumpStateDuration_secs_;
        [[serialize(10)]] float                     jumpCooldown_secs_;
        [[serialize(14)]] float                     jumpStaminaCost_;
        float                                       jumpCooldownRemaining_secs_ = 0.0f;
        [[serailize(0)]] float                      damageStateDuration_secs_;
        [[serailize(0)]] float                      avoidRollingStateDuration_secs_;
        [[serialize(0)]] float                      avoidRollingStaminaCost_;
        [[serialize(0)]] float                      deathStateDuration_secs_;
        [[serialize(0)]] float                      downStateDuration_secs_ = 13.6363636364f;
        [[serialize(16)]] float                     fallDownStateDuration_secs_ = 1.3333333333f;
        [[serialize(16)]] float                     getUpStateDuration_secs_    = 1.7666666667f;
        [[serialize(0)]] float                      reviveHealthRatio_      = 0.3f;
        bool                                         isDowned_               = false;

        [[serialize(0)]] float                          injuredHealthRatio_ = 0.3f;
        bool                                            wasInjured_         = false;
        rxcpp::subjects::subject<LibCore::Rx::unit>     onBecomeInjured_;
        rxcpp::subjects::subject<LibCore::Rx::unit>     onRecoverFromInjured_;

        std::queue<std::unique_ptr<IDamage>>   onDamagedStack_;

        // 初期所持(SwordManAvatarResource)はセーブにポーチが無いときだけ入れる
        [[serialize(21)]] ItemPouch pouch_;
        float     attackBuffRemaining_secs_ = 0.0f;
        float     attackBuffRate_ = 1.0f;

        void ConsumeStamina(float cost);
        
        

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(minHealth_));
            [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
            if (version <= 1) archive(CEREAL_NVP(health_));
            if (version >= 2) archive(CEREAL_NVP(currentHealth_));
            archive(CEREAL_NVP(maxStamina_));
            archive(CEREAL_NVP(stamina_));
            archive(CEREAL_NVP(staminaDrainPerSecond_));
            archive(CEREAL_NVP(staminaRegenPerSecond_));
            archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            archive(CEREAL_NVP(avoidRollingStaminaCost_));
            archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            archive(CEREAL_NVP(dashAttack_));
            archive(CEREAL_NVP(dashAttackLungeSpeed_secs_));
            archive(CEREAL_NVP(comboHitFeel_));
            archive(CEREAL_NVP(dashHitFeel_));
            archive(CEREAL_NVP(comboInputBufferWindow_secs_));
            archive(CEREAL_NVP(chargeAttackHoldThreshold_secs_));
            archive(CEREAL_NVP(chargeAttackMaxCharge_secs_));
            archive(CEREAL_NVP(chargeAttackMaxHold_secs_));
            archive(CEREAL_NVP(chargeAttack_));
            archive(CEREAL_NVP(chargeHitFeel_));
            archive(CEREAL_NVP(chargeAttackLungeStart_secs_));
            archive(CEREAL_NVP(chargeAttackLungeSpeed_));
            archive(CEREAL_NVP(chargeAttackStaminaCost_));
            archive(CEREAL_NVP(jumpAttack_));
            archive(CEREAL_NVP(jumpAttackHitFeel_));
            archive(CEREAL_NVP(jumpAttackWindup_secs_));
            archive(CEREAL_NVP(jumpAttackPlungeSpeed_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(lockOnAttackRotateSpeed_));
            archive(CEREAL_NVP(attackRotateSmoothTime_secs_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(jumpStateDuration_secs_));
            archive(CEREAL_NVP(jumpCooldown_secs_));
            archive(CEREAL_NVP(jumpStaminaCost_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(injuredHealthRatio_));
            archive(CEREAL_NVP(downStateDuration_secs_));
            archive(CEREAL_NVP(reviveHealthRatio_));
            archive(CEREAL_NVP(fallDownStateDuration_secs_));
            archive(CEREAL_NVP(getUpStateDuration_secs_));
            archive(CEREAL_NVP(quests_));
            archive(CEREAL_NVP(wallet_));
            archive(CEREAL_NVP(pouch_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            if (version >= 0) archive(CEREAL_NVP(maxHealth_));
            if (version >= 0) archive(CEREAL_NVP(minHealth_));
            [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
            if (version <= 1) archive(CEREAL_NVP(health_));
            if (version >= 2) archive(CEREAL_NVP(currentHealth_));
            if (version >= 5) archive(CEREAL_NVP(maxStamina_));
            if (version >= 5) archive(CEREAL_NVP(stamina_));
            if (version >= 5) archive(CEREAL_NVP(staminaDrainPerSecond_));
            if (version >= 5) archive(CEREAL_NVP(staminaRegenPerSecond_));
            if (version >= 5) archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            if (version >= 6) archive(CEREAL_NVP(avoidRollingStaminaCost_));
            if (version >= 1) archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(dashAttack_));
            if (version >= 8) archive(CEREAL_NVP(dashAttackLungeSpeed_secs_));
            if (version >= 9) archive(CEREAL_NVP(comboHitFeel_));
            // v18 のみ保持していた旧フィールド(HitFeelParam へ移動)を読み捨てる
            std::vector<float> comboAttackLungeSpeeds_;
            if (version == 18) archive(CEREAL_NVP(comboAttackLungeSpeeds_));
            if (version >= 9) archive(CEREAL_NVP(dashHitFeel_));
            if (version >= 9) archive(CEREAL_NVP(comboInputBufferWindow_secs_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackHoldThreshold_secs_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackMaxCharge_secs_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackMaxHold_secs_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttack_));
            if (version >= 11) archive(CEREAL_NVP(chargeHitFeel_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackLungeStart_secs_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackLungeSpeed_));
            if (version >= 11) archive(CEREAL_NVP(chargeAttackStaminaCost_));
            if (version >= 17) archive(CEREAL_NVP(jumpAttack_));
            if (version >= 17) archive(CEREAL_NVP(jumpAttackHitFeel_));
            if (version >= 17) archive(CEREAL_NVP(jumpAttackWindup_secs_));
            if (version >= 17) archive(CEREAL_NVP(jumpAttackPlungeSpeed_));
            if (version >= 0) archive(CEREAL_NVP(walkSpeed_));
            if (version >= 0) archive(CEREAL_NVP(runSpeed_));
            // v12 のみ保持していた旧フィールド(SwordManAvatarResource へ移動)を読み捨てる
            float walkAccelerationTime_secs_ = 0.0f;
            float runAccelerationTime_secs_  = 0.0f;
            if (version == 12) archive(CEREAL_NVP(walkAccelerationTime_secs_));
            if (version == 12) archive(CEREAL_NVP(runAccelerationTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(moveRotateSpeed_));
            if (version >= 7) archive(CEREAL_NVP(lockOnAttackRotateSpeed_));
            if (version >= 15) archive(CEREAL_NVP(attackRotateSmoothTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(jumpPower_));
            if (version >= 10) archive(CEREAL_NVP(jumpStateDuration_secs_));
            if (version >= 10) archive(CEREAL_NVP(jumpCooldown_secs_));
            if (version >= 14) archive(CEREAL_NVP(jumpStaminaCost_));
            if (version >= 0) archive(CEREAL_NVP(damageStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 3) archive(CEREAL_NVP(injuredHealthRatio_));
            if (version >= 4) archive(CEREAL_NVP(downStateDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(reviveHealthRatio_));
            if (version >= 16) archive(CEREAL_NVP(fallDownStateDuration_secs_));
            if (version >= 16) archive(CEREAL_NVP(getUpStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(quests_));
            if (version >= 20) archive(CEREAL_NVP(wallet_));
            if (version >= 21) archive(CEREAL_NVP(pouch_));
        }
    };
#pragma endregion 
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus, 21);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::IPlayerAvatarStatus, GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
#pragma endregion