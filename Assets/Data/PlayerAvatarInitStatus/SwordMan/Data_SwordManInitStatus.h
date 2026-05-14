#pragma once
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../Scripts/Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/BasicParams/AttackParam/AttackParam.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/EnahancePower/EnhancePower.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/SwordMan/Status/Quest/SwordMan_QuestGroup.h"
#include "../../../Scripts/Core/Game/StatusParameter/Health/Health.h"
#include "../../../Scripts/Core/Game/StatusParameter/MoveSpeed/MoveSpeed.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SWORD_MAN_INIT_STATUS_EXTENSION_LABEL = ".swordManInitStatus";
    
    class SwordManInitStatus final : public ScriptableObject
    {
    public:
        explicit SwordManInitStatus(const std::string& contentPath = "");

        [[nodiscard]] const GameCore::PlayerAvatar::SwordMan::QuestGroup&                        Quest                          () const { return *quests_; }
        
        [[nodiscard]] const GameCore::PlayerAvatar::EnhancePower& MaxEnhancePowerStack           () const { return maxEnhancePowerStack_;              }
        [[nodiscard]] const GameCore::PlayerAvatar::EnhancePower& ReinforceRequireEnhance        () const { return reinforceRequireEnhance_;              }
        [[nodiscard]] const GameCore::PlayerAvatar::EnhancePower& EnhancePowerStack              () const { return enhancePowerStack_; }
        [[nodiscard]] const GameCore::PlayerAvatar::EnhancePower& DecrementEnhancePowerStack_secs() const { return decrementEnhancePowerStack_secs_; }
        [[nodiscard]] const GameCore::StatusParameter::Health&    MaxHealth                      () const { return maxHealth_;    }
        [[nodiscard]] const GameCore::StatusParameter::Health&    MinHealth                      () const { return minHealth_;    }
        [[nodiscard]] const GameCore::StatusParameter::Health&    Health                         () const { return health_; }

        [[nodiscard]] const std::vector<GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower>>& ComboNormalAttack() const { return comboNormalAttack_; }
        [[nodiscard]] float                                ComboNormalAttackStateDuration_secs() const { return comboNormalAttackStateDuration_secs_; }
        [[nodiscard]] GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower> DashAttack() const { return dashAttack_;  }
        [[nodiscard]] GameCore::StatusParameter::MoveSpeed GetWalkSpeed        () const { return walkSpeed_;                }
        [[nodiscard]] GameCore::StatusParameter::MoveSpeed GetRunSpeed         () const { return runSpeed_ ;                }
        [[nodiscard]] float                                GetMoveRotateSpeed  () const { return moveRotateSpeed_;          }
        [[nodiscard]] float                                GetJumpPower        () const { return jumpPower_;                }
        [[nodiscard]] float                                GetJumpCooldown_secs() const { return jumpCooldown_secs_;        }
        [[nodiscard]] float                                OnEnableReinforceDuration_secs () const { return onEnableReinforceDuration_secs_; }
        [[nodiscard]] float                                OnDisableReinforceDuration_secs() const { return onDisableReinforceDuration_secs_; }
        [[nodiscard]] float                                DamageStateDuration_secs       () const { return damageStateDuration_secs_; }
        [[nodiscard]] float                                AvoidRollingStateDuration_secs () const { return avoidRollingStateDuration_secs_; }
        [[nodiscard]] float                                DeathStateDuration_secs        () const { return deathStateDuration_secs_; }
        [[nodiscard]] float                                ReinforceModeDuration_secs     () const { return reinforceModeDuration_secs_; }
        
    private:
        [[serialize(0)]] std::unique_ptr<GameCore::PlayerAvatar::SwordMan::QuestGroup> quests_;
        
        [[serialize(0)]] GameCore::StatusParameter::Health maxHealth_;
        [[serialize(0)]] GameCore::StatusParameter::Health minHealth_;
        [[serialize(0)]] GameCore::StatusParameter::Health health_;
        
        [[serialize(0)]] GameCore::PlayerAvatar::EnhancePower maxEnhancePowerStack_;
        [[serialize(0)]] GameCore::PlayerAvatar::EnhancePower enhancePowerStack_;
        [[serialize(0)]] GameCore::PlayerAvatar::EnhancePower reinforceRequireEnhance_;
        [[serialize(0)]] GameCore::PlayerAvatar::EnhancePower decrementEnhancePowerStack_secs_;
        
        [[serialize(0)]] std::vector<GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower>> comboNormalAttack_;
        [[serialize(2)]] float comboNormalAttackStateDuration_secs_;
        [[serialize(0)]] GameCore::PlayerAvatar::AttackParam<GameCore::Damage::PhysicsPower> dashAttack_;
        
        [[serialize(0)]] GameCore::StatusParameter::MoveSpeed walkSpeed_;
        [[serialize(0)]] GameCore::StatusParameter::MoveSpeed runSpeed_ ;
        [[serialize(0)]] float                                moveRotateSpeed_;
        [[serialize(0)]] float                                jumpPower_;
        [[serialize(0)]] float                                jumpCooldown_secs_;
        [[serialize(0)]] float                                onEnableReinforceDuration_secs_;
        [[serialize(0)]] float                                onDisableReinforceDuration_secs_;
        [[serialize(0)]] float                                damageStateDuration_secs_;
        [[serialize(0)]] float                                avoidRollingStateDuration_secs_;
        [[serialize(0)]] float                                deathStateDuration_secs_;
        [[serialize(0)]] float                                reinforceModeDuration_secs_;


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
            archive(CEREAL_NVP(maxEnhancePowerStack_));
            archive(CEREAL_NVP(enhancePowerStack_));
            archive(CEREAL_NVP(comboNormalAttackStateDuration_secs_));
            archive(CEREAL_NVP(dashAttack_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(jumpCooldown_secs_));
            archive(CEREAL_NVP(onEnableReinforceDuration_secs_));
            archive(CEREAL_NVP(onDisableReinforceDuration_secs_));
            archive(CEREAL_NVP(reinforceRequireEnhance_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(reinforceModeDuration_secs_));
            archive(CEREAL_NVP(quests_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(maxHealth_));
            if (version >= 0) archive(CEREAL_NVP(minHealth_));
            if (version >= 0) archive(CEREAL_NVP(health_));
            if (version >= 0) archive(CEREAL_NVP(maxEnhancePowerStack_));
            if (version >= 0) archive(CEREAL_NVP(enhancePowerStack_));
            if (version >= 2) archive(CEREAL_NVP(comboNormalAttackStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(dashAttack_));
            if (version >= 0) archive(CEREAL_NVP(walkSpeed_));
            if (version >= 0) archive(CEREAL_NVP(runSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveRotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(jumpPower_));
            if (version >= 1) archive(CEREAL_NVP(jumpCooldown_secs_));
            if (version >= 0) archive(CEREAL_NVP(onEnableReinforceDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(onDisableReinforceDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(reinforceRequireEnhance_));
            if (version >= 0) archive(CEREAL_NVP(damageStateDuration_secs_));
            if (version >= 3) archive(CEREAL_NVP(avoidRollingStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(reinforceModeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(quests_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(SwordManInitStatus, SWORD_MAN_INIT_STATUS_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SwordManInitStatus, 3);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SwordManInitStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::SwordManInitStatus);
#pragma endregion
