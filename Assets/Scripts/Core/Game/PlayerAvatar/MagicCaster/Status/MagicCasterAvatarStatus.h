#pragma once
#include <array>
#include <queue>

#include "../../../StatusParameter/Health/Health.h"
#include "../../../StatusParameter/Mana/Mana.h"
#include "../../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../StatusParameter/Stamina/Stamina.h"
#include "../../Item/Effect/IItemEffectTarget.h"
#include "../../Item/IItemReceiver.h"
#include "../../Item/ItemPouch.h"
#include "../../Status/IPlayerAvatarStatus.h"
#include "../Spell/MagicCasterSpellSlot.h"
#include "cereal/types/polymorphic.hpp"

#include "Engine/Core/Network/Object/NetworkObjectBase.h"
#include "Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Packages/R4/R4.h"
#include "../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../StateMachine/IReadOnlyPlayerAvatarStateMachine.h"
#include "../State/MagicCasterAvatarStateType.h"
#include "../../Quest/PlayerAvatar_IQuestGroup.h"
#include "../../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../Status/Event/PlayerAvatar_IStatusEvent.h"
#include "Quest/MagicCaster_QuestGroup.h"
#include "../../Wallet/PlayerAvatar_Wallet.h"

namespace GameCore::Magic
{
    class IMagicSpell;
}

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarStatus final : public NetworkObjectBase,
                                          public IPlayerAvatarStatus,
                                          public Item::IItemEffectTarget,
                                          public Item::IItemReceiver
    {
    public:
        MagicCasterAvatarStatus();
        ~MagicCasterAvatarStatus() override;
        MagicCasterAvatarStatus(MagicCasterAvatarStatus&&) noexcept = default;
        MagicCasterAvatarStatus& operator=(MagicCasterAvatarStatus&&) noexcept = default;
        void Init    () override;
        void OnUpdate() override;

        [[nodiscard]] IStatusEvent              & Event         () const override { return *event_        ; }
        [[nodiscard]] IQuestGroup                & Quest         () const override { return *quests_       ; }
        [[nodiscard]] Quest::ICompleteQuestGroup & CompletedQuest() const override { return *quests_       ; }
        [[nodiscard]] PlayerAvatar::Wallet       & Wallet        () const override { return *wallet_;        }
        [[nodiscard]] ItemPouch                  & Pouch         ()       override { return pouch_;          }
        [[nodiscard]] const ItemPouch            & Pouch         () const override { return pouch_;          }
        [[nodiscard]] int ReceivableCount(const Asset::ItemData& item) const override { return pouch_.ReceivableCount(item); }
        int ReceiveItem(const std::shared_ptr<Asset::ItemData>& item, const int count) override { return pouch_.Add(item, count); }

        [[nodiscard]] const StatusParameter::Health&                     MaxHealth() const override { return maxHealth_; }
        [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Health> OnChangeHealth() const override { return onChangeHealth_.AsObservable(); }
        [[nodiscard]] StatusParameter::Health                            Health() const override { return currentHealth_->Get(); }
        [[nodiscard]] bool                                               IsDeath() const override { return minHealth_ >= currentHealth_->Get(); }

        [[nodiscard]] const StatusParameter::Stamina&                                MaxStamina() const override { return maxStamina_; }
        [[nodiscard]] NanamiEngine::R4::ReadOnlyReactiveProperty<StatusParameter::Stamina> Stamina   () const override { return stamina_.AsReadOnly(); }
        [[nodiscard]] bool                                                           CanRun    () const override { return !isStaminaExhausted_; }

        [[nodiscard]] StatusParameter::MoveSpeed GetWalkSpeed         () const override { return walkSpeed_; }
        [[nodiscard]] StatusParameter::MoveSpeed GetRunSpeed          () const override { return runSpeed_; }
        [[nodiscard]] float                      GetMoveRotateSpeed  () const override { return moveRotateSpeed_; }
        [[nodiscard]] float                      GetAimRotateSpeed   () const          { return aimRotateSpeed_; }
        [[nodiscard]] float                      GetJumpPower      () const override { return jumpPower_; }
        [[nodiscard]] float                      GetJumpStateDuration_secs() const override { return jumpStateDuration_secs_; }

        void SetStateMachine(const IReadOnlyPlayerAvatarStateMachine<MagicCasterAvatarStateType>& stateMachine) { stateMachine_ = &stateMachine; }

        [[nodiscard]] bool CanJump() const { return jumpCooldownRemaining_secs_ <= 0.0f && stamina_.Value() >= StatusParameter::Stamina(jumpStaminaCost_); }
        void StartJumpCooldown() { jumpCooldownRemaining_secs_ = jumpCooldown_secs_; }
        void ConsumeJumpStamina() { ConsumeStamina(jumpStaminaCost_); }

        [[nodiscard]] bool  CanAvoidRolling() const { return stamina_.Value() >= StatusParameter::Stamina(avoidRollingStaminaCost_); }
        [[nodiscard]] float AvoidRollingStateDuration_secs() const { return avoidRollingStateDuration_secs_; }
        void ConsumeAvoidRollingStamina() { ConsumeStamina(avoidRollingStaminaCost_); }

        [[nodiscard]] const StatusParameter::Mana&                                MaxMana() const { return maxMana_; }
        [[nodiscard]] NanamiEngine::R4::ReadOnlyReactiveProperty<StatusParameter::Mana> Mana   () const { return mana_.AsReadOnly(); }
        /** @brief slot は MagicCasterSpellSlot.h の枠番号。クールタイム中か MP が足りなければ false */
        [[nodiscard]] bool CanCast(int slot, const GameCore::Magic::IMagicSpell& spell) const;
        /** @brief MP を払い、その枠のクールタイムを始める */
        void BeginCast(int slot, const GameCore::Magic::IMagicSpell& spell);
        [[nodiscard]] float CooldownRemaining_secs(int slot) const;
        /** @brief 残りクールタイムの割合。1 で始まったばかり、0 で使える */
        [[nodiscard]] float CooldownRemainingRate(int slot) const;

        /** @brief 体力を amount だけ戻す。最大値で頭打ち、死亡中は何もしない */
        void Heal(StatusParameter::Health amount) override;
        void RestoreStamina(float amount) override;
        /** @brief 魔法の威力の倍率を duration_secs のあいだ差し替える。重ねがけは上書き */
        void ApplyAttackBuff(float rate, float duration_secs) override;
        [[nodiscard]] float AttackPowerRate() const { return attackBuffRemaining_secs_ > 0.0f ? attackBuffRate_ : 1.0f; }

        [[nodiscard]] float DamageStateDuration_secs() const { return damageStateDuration_secs_; }
        [[nodiscard]] float DeathStateDuration_secs () const { return deathStateDuration_secs_;  }

        [[nodiscard]] bool IsDamaged() const;
        void AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;
        void ApplyDamage();
        void DiscardDamage();

    private:
        class StatusEvent final : public IStatusEvent
        {
        public:
            [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Health> OnDamage() const override { return onDamage_.AsObservable(); }
            NanamiEngine::R4::Subject<StatusParameter::Health> onDamage_;
        };

        std::shared_ptr<StatusEvent> event_ = std::make_shared<StatusEvent>();
        [[serialize(3)]] std::unique_ptr<QuestGroup> quests_ = std::make_unique<QuestGroup>();
        [[serialize(1)]] std::shared_ptr<PlayerAvatar::Wallet> wallet_;
        // 初期所持は無い。店で買うか拾うかで増える
        [[serialize(4)]] ItemPouch pouch_;

        [[serialize(0)]] StatusParameter::Health maxHealth_;
        [[serialize(0)]] StatusParameter::Health minHealth_;
        NanamiEngine::R4::Subject<StatusParameter::Health> onChangeHealth_;
        [[serialize(0)]] SyncParam<StatusParameter::Health> currentHealth_ = SyncParamFactory::Create<StatusParameter::Health>(this, StatusParameter::Health(100));

        [[serialize(0)]] StatusParameter::Stamina maxStamina_;
        [[serialize(0)]] NanamiEngine::R4::SerializableReactiveProperty<StatusParameter::Stamina> stamina_;
        [[serialize(0)]] float staminaDrainPerSecond_;
        [[serialize(0)]] float staminaRegenPerSecond_;
        [[serialize(0)]] float minStaminaRatioToResumeRun_ = 0.3f;
        bool isStaminaExhausted_ = false;
        const IReadOnlyPlayerAvatarStateMachine<MagicCasterAvatarStateType>* stateMachine_ = nullptr;

        [[serialize(0)]] StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] StatusParameter::MoveSpeed runSpeed_;
        [[serialize(0)]] float moveRotateSpeed_;
        // NOTE: 狙いへの向き直り(FaceAimTarget)用。移動の回転速度とは別にする
        float aimRotateSpeed_ = 6.2f;
        [[serialize(0)]] float jumpPower_;
        [[serialize(0)]] float jumpStateDuration_secs_;
        [[serialize(0)]] float jumpCooldown_secs_;
        [[serialize(0)]] float jumpStaminaCost_;
        float jumpCooldownRemaining_secs_ = 0.0f;
        [[serialize(5)]] float avoidRollingStateDuration_secs_;
        [[serialize(5)]] float avoidRollingStaminaCost_;

        [[serialize(0)]] float damageStateDuration_secs_;
        [[serialize(0)]] float deathStateDuration_secs_;

        [[serialize(2)]] StatusParameter::Mana maxMana_;
        [[serialize(2)]] NanamiEngine::R4::SerializableReactiveProperty<StatusParameter::Mana> mana_;
        [[serialize(2)]] float manaRegenPerSecond_;
        std::array<float, SPELL_SLOT_COUNT> cooldownRemaining_secs_ {};
        std::array<float, SPELL_SLOT_COUNT> cooldownDuration_secs_ {};

        float attackBuffRate_ = 1.0f;
        float attackBuffRemaining_secs_ = 0.0f;
        // NOTE: 被ダメージ(ApplyDamage)から数える。無敵中のダメージはスタックに積まずに捨てる
        float invincibleDuration_secs_  = 2.0f;
        float invincibleRemaining_secs_ = 0.0f;

        std::queue<std::unique_ptr<IDamage>> onDamagedStack_;

        void ConsumeStamina(float cost);
        [[nodiscard]] static bool IsValidSpellSlot(int slot) { return slot >= 0 && slot < SPELL_SLOT_COUNT; }

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(minHealth_));
            archive(CEREAL_NVP(currentHealth_));
            archive(CEREAL_NVP(maxStamina_));
            archive(CEREAL_NVP(stamina_));
            archive(CEREAL_NVP(staminaDrainPerSecond_));
            archive(CEREAL_NVP(staminaRegenPerSecond_));
            archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(jumpStateDuration_secs_));
            archive(CEREAL_NVP(jumpCooldown_secs_));
            archive(CEREAL_NVP(jumpStaminaCost_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(wallet_));
            archive(CEREAL_NVP(maxMana_));
            archive(CEREAL_NVP(mana_));
            archive(CEREAL_NVP(manaRegenPerSecond_));
            archive(CEREAL_NVP(quests_));
            archive(CEREAL_NVP(pouch_));
            archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            archive(CEREAL_NVP(avoidRollingStaminaCost_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(minHealth_));
            archive(CEREAL_NVP(currentHealth_));
            archive(CEREAL_NVP(maxStamina_));
            archive(CEREAL_NVP(stamina_));
            archive(CEREAL_NVP(staminaDrainPerSecond_));
            archive(CEREAL_NVP(staminaRegenPerSecond_));
            archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(jumpStateDuration_secs_));
            archive(CEREAL_NVP(jumpCooldown_secs_));
            archive(CEREAL_NVP(jumpStaminaCost_));
            if (version <= 1)
            {
                // v1 までは魔法弾1種をスタミナで撃っていた。魔法ごとの MP とクールタイムに移ったので読み捨てる
                Damage::PhysicsPower castDamage_;
                float castStaminaCost_   = 0.0f;
                float castCooldown_secs_ = 0.0f;
                archive(CEREAL_NVP(castDamage_));
                archive(CEREAL_NVP(castStaminaCost_));
                archive(CEREAL_NVP(castCooldown_secs_));
            }
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(wallet_));
            if (version >= 2) archive(CEREAL_NVP(maxMana_));
            if (version >= 2) archive(CEREAL_NVP(mana_));
            if (version >= 2) archive(CEREAL_NVP(manaRegenPerSecond_));
            if (version >= 3) archive(CEREAL_NVP(quests_));
            if (version >= 4) archive(CEREAL_NVP(pouch_));
            if (version >= 5) archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            if (version >= 5) archive(CEREAL_NVP(avoidRollingStaminaCost_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus, 5);
#pragma endregion
