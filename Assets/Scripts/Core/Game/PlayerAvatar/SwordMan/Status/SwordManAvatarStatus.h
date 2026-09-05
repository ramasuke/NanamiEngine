#pragma once
#include "../../../StatusParameter/Health/Health.h"
#include "../../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../StatusParameter/Stamina/Stamina.h"
#include "../../Status/IPlayerAvatarStatus.h"
#include "../../Status/BasicParams/AttackParam/AttackParam.h"
#include "../../Status/EnahancePower/EnhancePower.h"
#include "cereal/types/polymorphic.hpp"
#include <queue>

#include "../../../../../../../Engine/Core/Network/Object/NetworkObjectBase.h"
#include "../../../../../../../Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "Event/SwordManAvatarStatusEvent.h"
#include "Quest/SwordMan_QuestGroup.h"

namespace NanamiEngine::Module::Asset
{
    class SwordManInitStatus;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus final : public NetworkObjectBase,
                                       public IPlayerAvatarStatus
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
                      void                                                           SetIsRunning(bool isRunning) { isRunning_ = isRunning; }

        [[nodiscard]] const std::vector<AttackParam<Damage::PhysicsPower>>& ComboNormalAttack() const { return comboNormalAttack_; }
        [[nodiscard]] float                             ComboNormalAttackStateDuration_secs  () const { return comboNormalAttackStateDuration_secs_; }
        [[nodiscard]] float                             AttackedShockedStateDuration_secs    () const { return attackedShockedStateDuration_secs_; }
        [[nodiscard]] StatusParameter::MoveSpeed        GetWalkSpeed                         () const override { return walkSpeed_;                }
        [[nodiscard]] StatusParameter::MoveSpeed        GetRunSpeed                          () const override { return runSpeed_ ;                }
        [[nodiscard]] float                             GetMoveRotateSpeed                   () const override { return moveRotateSpeed_;          }
        [[nodiscard]] float                             GetJumpPower                         () const override { return jumpPower_;                }
        [[nodiscard]] float                             GetJumpCooldown_secs                 () const override { return jumpCooldown_secs_;        }
        [[nodiscard]] AttackParam<Damage::PhysicsPower> DashAttack                           () const          { return dashAttack_;  }
        [[nodiscard]] bool                              IsDamaged                            () const;
        [[nodiscard]] float                             DamageStateDuration_secs             () const   { return damageStateDuration_secs_; }
        [[nodiscard]] float                             AvoidRollingStateDuration_secs       () const   { return avoidRollingStateDuration_secs_; }
        [[nodiscard]] float                             DeathStateDuration_secs              () const   { return deathStateDuration_secs_; }
        [[nodiscard]] float                             DownStateDuration_secs               () const   { return downStateDuration_secs_; }
                      void                              AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;
                      void                              ApplyDamage();
                      void                              DiscardDamage();
        
        
    private:
        std::shared_ptr<StatusEvent> event_;
        [[serialize(0)]] std::unique_ptr<QuestGroup> quests_;
        
        [[serialize(0)]] StatusParameter::Health maxHealth_;
        [[serialize(0)]] StatusParameter::Health minHealth_;
        rxcpp::subjects::subject<StatusParameter::Health> onChangeHealth_;
        [[serialize(0)]] SyncParam<StatusParameter::Health> currentHealth_ = SyncParamFactory::Create<StatusParameter::Health>(this, StatusParameter::Health(100));
        
        [[serialize(0)]] StatusParameter::Stamina maxStamina_;
        [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Stamina> stamina_;
        [[serialize(0)]] float staminaDrainPerSecond_;
        [[serialize(0)]] float staminaRegenPerSecond_;
        [[serialize(0)]] float minStaminaRatioToResumeRun_ = 0.3f;
        bool isRunning_          = false;
        bool isStaminaExhausted_ = false;

        [[serialize(0)]] std::vector<AttackParam<Damage::PhysicsPower>> comboNormalAttack_;
        [[serialize(0)]] float comboNormalAttackStateDuration_secs_;
        [[serialize(0)]] float attackedShockedStateDuration_secs_;
        [[serialize(0)]] AttackParam<Damage::PhysicsPower> dashAttack_;
        
        [[serialize(0)]] StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] StatusParameter::MoveSpeed runSpeed_ ;
        [[serialize(0)]] float                      moveRotateSpeed_;
        [[serialize(0)]] float                      jumpPower_;
        [[serialize(0)]] float                      jumpCooldown_secs_;
        [[serailize(0)]] float                      damageStateDuration_secs_;
        [[serailize(0)]] float                      avoidRollingStateDuration_secs_;
        [[serialize(0)]] float                      deathStateDuration_secs_;
        [[serialize(0)]] float                      downStateDuration_secs_ = 15.0f;
        [[serialize(0)]] float                      reviveHealthRatio_      = 0.3f;
        bool                                         isDowned_               = false;

        [[serialize(0)]] float                          injuredHealthRatio_ = 0.3f;
        bool                                            wasInjured_         = false;
        rxcpp::subjects::subject<LibCore::Rx::unit>     onBecomeInjured_;
        rxcpp::subjects::subject<LibCore::Rx::unit>     onRecoverFromInjured_;

        std::queue<std::unique_ptr<IDamage>>   onDamagedStack_;
        
        

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
            archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            archive(CEREAL_NVP(dashAttack_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(injuredHealthRatio_));
            archive(CEREAL_NVP(downStateDuration_secs_));
            archive(CEREAL_NVP(reviveHealthRatio_));
            archive(CEREAL_NVP(quests_));
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
            if (version >= 1) archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(dashAttack_));
            if (version >= 0) archive(CEREAL_NVP(walkSpeed_));
            if (version >= 0) archive(CEREAL_NVP(runSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveRotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(jumpPower_));
            if (version >= 0) archive(CEREAL_NVP(damageStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 3) archive(CEREAL_NVP(injuredHealthRatio_));
            if (version >= 4) archive(CEREAL_NVP(downStateDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(reviveHealthRatio_));
            if (version >= 0) archive(CEREAL_NVP(quests_));
        }
    };
#pragma endregion 
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus, 5);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::IPlayerAvatarStatus, GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
#pragma endregion