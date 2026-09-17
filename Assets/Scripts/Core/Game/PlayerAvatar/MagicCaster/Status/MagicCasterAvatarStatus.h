#pragma once
#include <queue>

#include "../../../StatusParameter/Health/Health.h"
#include "../../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../StatusParameter/Stamina/Stamina.h"
#include "../../Status/IPlayerAvatarStatus.h"
#include "cereal/types/polymorphic.hpp"

#include "../../../../../../../Engine/Core/Network/Object/NetworkObjectBase.h"
#include "../../../../../../../Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../StateMachine/IReadOnlyPlayerAvatarStateMachine.h"
#include "../State/MagicCasterAvatarStateType.h"
#include "../../Quest/PlayerAvatar_IQuestGroup.h"
#include "../../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../Status/Event/PlayerAvatar_IStatusEvent.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarStatus final : public NetworkObjectBase,
                                          public IPlayerAvatarStatus
    {
    public:
        MagicCasterAvatarStatus();
        ~MagicCasterAvatarStatus() override;
        void Init    () override;
        void OnUpdate() override;

        [[nodiscard]] IStatusEvent              & Event         () const override { return *event_        ; }
        [[nodiscard]] IQuestGroup                & Quest         () const override { return *quest_        ; }
        [[nodiscard]] Quest::ICompleteQuestGroup & CompletedQuest() const override { return *completeQuest_; }

        [[nodiscard]] const StatusParameter::Health&                     MaxHealth() const override { return maxHealth_; }
        [[nodiscard]] rxcpp::observable<StatusParameter::Health> OnChangeHealth() const override { return onChangeHealth_.get_observable(); }
        [[nodiscard]] StatusParameter::Health                            Health() const override { return currentHealth_->Get(); }
        [[nodiscard]] bool                                               IsDeath() const override { return minHealth_ >= currentHealth_->Get(); }

        [[nodiscard]] const StatusParameter::Stamina&                                MaxStamina() const override { return maxStamina_; }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Stamina> Stamina   () const override { return stamina_.AsReadOnly(); }
        [[nodiscard]] bool                                                           CanRun    () const override { return !isStaminaExhausted_; }

        [[nodiscard]] StatusParameter::MoveSpeed GetWalkSpeed         () const override { return walkSpeed_; }
        [[nodiscard]] StatusParameter::MoveSpeed GetRunSpeed          () const override { return runSpeed_; }
        [[nodiscard]] float                      GetMoveRotateSpeed  () const override { return moveRotateSpeed_; }
        [[nodiscard]] float                      GetJumpPower        () const override { return jumpPower_; }
        [[nodiscard]] float                      GetJumpStateDuration_secs() const override { return jumpStateDuration_secs_; }

        void SetStateMachine(const IReadOnlyPlayerAvatarStateMachine<MagicCasterAvatarStateType>& stateMachine) { stateMachine_ = &stateMachine; }

        [[nodiscard]] bool CanJump() const { return jumpCooldownRemaining_secs_ <= 0.0f && stamina_.get() >= StatusParameter::Stamina(jumpStaminaCost_); }
        void StartJumpCooldown() { jumpCooldownRemaining_secs_ = jumpCooldown_secs_; }
        void ConsumeJumpStamina() { ConsumeStamina(jumpStaminaCost_); }

        [[nodiscard]] bool CanCast() const { return castCooldownRemaining_secs_ <= 0.0f && stamina_.get() >= StatusParameter::Stamina(castStaminaCost_); }
        void StartCastCooldown() { castCooldownRemaining_secs_ = castCooldown_secs_; }
        void ConsumeCastStamina() { ConsumeStamina(castStaminaCost_); }
        [[nodiscard]] const Damage::PhysicsPower& CastDamage() const { return castDamage_; }

        [[nodiscard]] float DamageStateDuration_secs() const { return damageStateDuration_secs_; }
        [[nodiscard]] float DeathStateDuration_secs () const { return deathStateDuration_secs_;  }

        [[nodiscard]] bool IsDamaged() const;
        void AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;
        void ApplyDamage();
        void DiscardDamage();

    private:
        class NoOpQuestGroup final : public IQuestGroup
        {
        public:
            void Subscribe(const std::shared_ptr<QuestBase>&) override {}
        };

        class NoOpCompleteQuestGroup final : public Quest::ICompleteQuestGroup
        {
        public:
            void CompleteQuest(const QuestType&) override {}
            [[nodiscard]] bool CheckCompleted(const QuestType&) const override { return false; }
        };

        class StatusEvent final : public IStatusEvent
        {
        public:
            [[nodiscard]] rxcpp::observable<StatusParameter::Health> OnDamage() const override { return onDamage_.get_observable(); }
            rxcpp::subjects::subject<StatusParameter::Health> onDamage_;
        };

        std::shared_ptr<StatusEvent> event_ = std::make_shared<StatusEvent>();
        std::unique_ptr<NoOpQuestGroup> quest_ = std::make_unique<NoOpQuestGroup>();
        std::unique_ptr<NoOpCompleteQuestGroup> completeQuest_ = std::make_unique<NoOpCompleteQuestGroup>();

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
        const IReadOnlyPlayerAvatarStateMachine<MagicCasterAvatarStateType>* stateMachine_ = nullptr;

        [[serialize(0)]] StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] StatusParameter::MoveSpeed runSpeed_;
        [[serialize(0)]] float moveRotateSpeed_;
        [[serialize(0)]] float jumpPower_;
        [[serialize(0)]] float jumpStateDuration_secs_;
        [[serialize(0)]] float jumpCooldown_secs_;
        [[serialize(0)]] float jumpStaminaCost_;
        float jumpCooldownRemaining_secs_ = 0.0f;

        [[serialize(0)]] Damage::PhysicsPower castDamage_;
        [[serialize(0)]] float castStaminaCost_;
        [[serialize(0)]] float castCooldown_secs_;
        float castCooldownRemaining_secs_ = 0.0f;

        [[serialize(0)]] float damageStateDuration_secs_;
        [[serialize(0)]] float deathStateDuration_secs_;

        std::queue<std::unique_ptr<IDamage>> onDamagedStack_;

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
            archive(CEREAL_NVP(castDamage_));
            archive(CEREAL_NVP(castStaminaCost_));
            archive(CEREAL_NVP(castCooldown_secs_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
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
            archive(CEREAL_NVP(castDamage_));
            archive(CEREAL_NVP(castStaminaCost_));
            archive(CEREAL_NVP(castCooldown_secs_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::IPlayerAvatarStatus, GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus);
#pragma endregion
