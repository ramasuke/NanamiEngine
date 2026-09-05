#pragma once
#include "../../../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../../Scripts/Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../../../Scripts/Core/Game/PlayerAvatar/Status/BasicParams/AttackParam/AttackParam.h"
#include "../../../../Scripts/Core/Game/PlayerAvatar/Status/EnahancePower/EnhancePower.h"
#include "../../../../Scripts/Core/Game/PlayerAvatar/SwordMan/Status/Quest/SwordMan_QuestGroup.h"
#include "../../../../Scripts/Core/Game/StatusParameter/Health/Health.h"
#include "../../../../Scripts/Core/Game/StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../../Scripts/Core/Game/StatusParameter/Stamina/Stamina.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SWORD_MAN_INIT_STATUS_EXTENSION_LABEL = ".swordManInitStatus";
    
    class SwordManInitStatus final : public ScriptableObject
    {
    public:
        explicit SwordManInitStatus(const std::string& contentPath = "");

        [[nodiscard]] const GameCore::PlayerAvatar::SwordMan::QuestGroup&                        Quest                          () const { return *quests_; }

        [[nodiscard]] const GameCore::StatusParameter::Stamina& MaxStamina                 () const { return maxStamina_; }
        [[nodiscard]] float                                     StaminaDrainPerSecond      () const { return staminaDrainPerSecond_; }
        [[nodiscard]] float                                     StaminaRegenPerSecond      () const { return staminaRegenPerSecond_; }
        [[nodiscard]] float                                     MinStaminaRatioToResumeRun () const { return minStaminaRatioToResumeRun_; }
        [[nodiscard]] const GameCore::StatusParameter::Health&    MaxHealth                      () const { return maxHealth_;    }
        [[nodiscard]] const GameCore::StatusParameter::Health&    MinHealth                      () const { return minHealth_;    }
        [[nodiscard]] const GameCore::StatusParameter::Health&    Health                         () const { return health_; }

        [[nodiscard]] const std::vector<GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower>>& ComboNormalAttack() const { return comboNormalAttack_; }
        [[nodiscard]] float                                ComboNormalAttackStateDuration_secs() const { return comboNormalAttackStateDuration_secs_; }
        [[nodiscard]] float                                AttackedShockedStateDuration_secs_() const { return attackedShockedStateDuration_secs_; }
        [[nodiscard]] GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower> DashAttack() const { return dashAttack_;  }
        [[nodiscard]] GameCore::StatusParameter::MoveSpeed GetWalkSpeed        () const { return walkSpeed_;                }
        [[nodiscard]] GameCore::StatusParameter::MoveSpeed GetRunSpeed         () const { return runSpeed_ ;                }
        [[nodiscard]] float                                GetMoveRotateSpeed  () const { return moveRotateSpeed_;          }
        [[nodiscard]] float                                GetJumpPower        () const { return jumpPower_;                }
        [[nodiscard]] float                                GetJumpCooldown_secs() const { return jumpCooldown_secs_;        }
        [[nodiscard]] float                                DamageStateDuration_secs       () const { return damageStateDuration_secs_; }
        [[nodiscard]] float                                AvoidRollingStateDuration_secs () const { return avoidRollingStateDuration_secs_; }
        [[nodiscard]] float                                DeathStateDuration_secs        () const { return deathStateDuration_secs_; }
        [[nodiscard]] float                                GetInjuredHealthRatio          () const { return injuredHealthRatio_; }

    private:
        [[serialize(0)]] std::unique_ptr<GameCore::PlayerAvatar::SwordMan::QuestGroup> quests_;
        
        [[serialize(0)]] GameCore::StatusParameter::Health maxHealth_;
        [[serialize(0)]] GameCore::StatusParameter::Health minHealth_;
        [[serialize(0)]] GameCore::StatusParameter::Health health_;

        [[serialize(0)]] GameCore::StatusParameter::Stamina maxStamina_;
        [[serialize(0)]] float                              staminaDrainPerSecond_;
        [[serialize(0)]] float                              staminaRegenPerSecond_;
        [[serialize(0)]] float                              minStaminaRatioToResumeRun_ = 0.3f;

        [[serialize(0)]] std::vector<GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower>> comboNormalAttack_;
        [[serialize(2)]] float comboNormalAttackStateDuration_secs_;
        [[serialize(4)]] float attackedShockedStateDuration_secs_;
        [[serialize(0)]] GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower> dashAttack_;
        
        [[serialize(0)]] GameCore::StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] GameCore::StatusParameter::MoveSpeed runSpeed_ ;
        [[serialize(0)]] float                                moveRotateSpeed_;
        [[serialize(0)]] float                                jumpPower_;
        [[serialize(0)]] float                                jumpCooldown_secs_;
        [[serialize(0)]] float                                damageStateDuration_secs_;
        [[serialize(0)]] float                                avoidRollingStateDuration_secs_;
        [[serialize(0)]] float                                deathStateDuration_secs_;
        [[serialize(5)]] float                                injuredHealthRatio_ = 0.3f;


#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(minHealth_));
            archive(CEREAL_NVP(health_));
            archive(CEREAL_NVP(maxStamina_));
            archive(CEREAL_NVP(staminaDrainPerSecond_));
            archive(CEREAL_NVP(staminaRegenPerSecond_));
            archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            archive(CEREAL_NVP(comboNormalAttackStateDuration_secs_));
            archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            archive(CEREAL_NVP(dashAttack_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(jumpCooldown_secs_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(injuredHealthRatio_));
            archive(CEREAL_NVP(quests_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(maxHealth_));
            if (version >= 0) archive(CEREAL_NVP(minHealth_));
            if (version >= 0) archive(CEREAL_NVP(health_));
            if (version >= 6) archive(CEREAL_NVP(maxStamina_));
            if (version >= 6) archive(CEREAL_NVP(staminaDrainPerSecond_));
            if (version >= 6) archive(CEREAL_NVP(staminaRegenPerSecond_));
            if (version >= 6) archive(CEREAL_NVP(minStaminaRatioToResumeRun_));
            if (version >= 2) archive(CEREAL_NVP(comboNormalAttackStateDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(attackedShockedStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(dashAttack_));
            if (version >= 0) archive(CEREAL_NVP(walkSpeed_));
            if (version >= 0) archive(CEREAL_NVP(runSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveRotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(jumpPower_));
            if (version >= 1) archive(CEREAL_NVP(jumpCooldown_secs_));
            if (version >= 0) archive(CEREAL_NVP(damageStateDuration_secs_));
            if (version >= 3) archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 5) archive(CEREAL_NVP(injuredHealthRatio_));
            if (version >= 0) archive(CEREAL_NVP(quests_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(SwordManInitStatus, SWORD_MAN_INIT_STATUS_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SwordManInitStatus, 6);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SwordManInitStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::SwordManInitStatus);
#pragma endregion
