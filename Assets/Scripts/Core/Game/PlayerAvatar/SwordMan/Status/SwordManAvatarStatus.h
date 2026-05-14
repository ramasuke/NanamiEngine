#pragma once
#include "../../../StatusParameter/Health/Health.h"
#include "../../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../Status/IPlayerAvatarStatus.h"
#include "../../Status/BasicParams/AttackParam/AttackParam.h"
#include "../../Status/EnahancePower/EnhancePower.h"
#include "cereal/types/polymorphic.hpp"
#include <queue>

#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "Event/SwordManAvatarStatusEvent.h"
#include "Quest/SwordMan_QuestGroup.h"
#include "../../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Asset
{
    class SwordManInitStatus;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus final : public IPlayerAvatarStatus
    {
    public:
        SwordManAvatarStatus();
        explicit SwordManAvatarStatus(const Asset::SwordManInitStatus& initStatus);
        ~SwordManAvatarStatus() override;
        void Init    () override;
        void OnUpdate() override;
        
        [[nodiscard]] IObservableStatusEvent    & Observable() const          { return *event_ ; }
        [[nodiscard]] State::IStatusEventSubject& Subject   () const          { return *event_ ; }
        [[nodiscard]] IStatusEvent              & Event     () const override { return *event_ ; }
        [[nodiscard]] QuestGroup                & Quest     () const override { return *quests_; }
        
        [[nodiscard]] const StatusParameter::Health&                                MaxHealth() const override { return maxHealth_;           }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Health> Health   () const override { return health_.AsReadOnly(); }
        [[nodiscard]] bool                                               IsDeath  () const          { return minHealth_ >= health_.get();   }          
        [[nodiscard]] const EnhancePower&                                MaxEnhancePowerStack() const override { return maxEnhancePowerStack_;           }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<EnhancePower> EnhancePowerStack   () const override { return enhancePowerStack_   .AsReadOnly(); }
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<bool>         IsEnableReinforce   () const override { return isReinforceMode_.AsReadOnly(); }

        [[nodiscard]] const std::vector<AttackParam<Damage::PhysicsPower>>& ComboNormalAttack() const { return comboNormalAttack_; }
        [[nodiscard]] float                             ComboNormalAttackStateDuration_secs() const { return comboNormalAttackStateDuration_secs_; }
        [[nodiscard]] StatusParameter::MoveSpeed        GetWalkSpeed            () const override { return walkSpeed_;                }
        [[nodiscard]] StatusParameter::MoveSpeed        GetRunSpeed             () const override { return runSpeed_ ;                }
        [[nodiscard]] float                             GetMoveRotateSpeed      () const override { return moveRotateSpeed_;          }
        [[nodiscard]] float                             GetJumpPower            () const override { return jumpPower_;                }
        [[nodiscard]] float                             GetJumpCooldown_secs    () const override { return jumpCooldown_secs_;        }
        [[nodiscard]] AttackParam<Damage::PhysicsPower> DashAttack              () const          { return dashAttack_;  }
        [[nodiscard]] bool                              CanReinforce            () const          { return reinforceRequireEnhance_ <= enhancePowerStack_.get(); }
                      void                              OnEnableReinforce       (); 
                      void                              OnDisableReinforce      (); 
        [[nodiscard]] float                             OnEnableReinforceDuration_secs () const   { return onEnableReinforceDuration_secs_; }
        [[nodiscard]] float                             OnDisableReinforceDuration_secs() const   { return onDisableReinforceDuration_secs_; }
        [[nodiscard]] bool                              IsDamaged                      () const;
        [[nodiscard]] float                             DamageStateDuration_secs       () const   { return damageStateDuration_secs_; }
        [[nodiscard]] float                             AvoidRollingStateDuration_secs () const   { return avoidRollingStateDuration_secs_; }
        [[nodiscard]] float                             DeathStateDuration_secs        () const   { return deathStateDuration_secs_; }
        [[nodiscard]] float                             ReinforceModeDuring_secs       () const override { return reinforceModeDuring_secs_; }
        [[nodiscard]] float                             ReinforceModeDuration_secs     () const override { return reinforceModeDuration_secs_; }
        [[nodiscard]] bool                              IsOnDisableReinforceMode       () const override { return reinforceModeDuring_secs_ > reinforceModeDuration_secs_; }
                      void                              AddEnhancePowerStack           (const EnhancePower& enhancePower);
                      void                              AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;
                      void                              ApplyDamage();
                      void                              DiscardDamage();
        
        
    private:
        std::shared_ptr<StatusEvent> event_;
        [[serialize(0)]] std::unique_ptr<QuestGroup> quests_;
        
        [[serialize(0)]] StatusParameter::Health maxHealth_;
        [[serialize(0)]] StatusParameter::Health minHealth_; 
        [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Health> health_;
        
        [[serialize(0)]] EnhancePower maxEnhancePowerStack_;
        [[serialize(0)]] LibCore::Rx::SerializableSubject<EnhancePower> enhancePowerStack_;

        [[serialize(0)]] EnhancePower                                   decrementEnhancePowerStack_secs_;

        [[serialize(0)]] std::vector<AttackParam<Damage::PhysicsPower>> comboNormalAttack_;
        [[serialize(0)]] float comboNormalAttackStateDuration_secs_;
        [[serialize(0)]] AttackParam<Damage::PhysicsPower> dashAttack_;
        
        [[serialize(0)]] StatusParameter::MoveSpeed        walkSpeed_;
        [[serialize(0)]] StatusParameter::MoveSpeed        runSpeed_ ;
        [[serialize(0)]] float                             moveRotateSpeed_;
        [[serialize(0)]] float                             jumpPower_;
        [[serialize(0)]] float                             jumpCooldown_secs_;
        [[serialize(0)]] float                             onEnableReinforceDuration_secs_;
        [[serialize(0)]] float                             onDisableReinforceDuration_secs_;
        [[serialize(0)]] EnhancePower                      reinforceRequireEnhance_;
        [[serailize(0)]] float                             damageStateDuration_secs_;
        [[serailize(0)]] float                             avoidRollingStateDuration_secs_;
        [[serialize(0)]] float                             deathStateDuration_secs_;
        [[serailize(0)]] float                             reinforceModeDuring_secs_ ;
        [[serailize(0)]] float                             reinforceModeDuration_secs_;
        
        LibCore::Rx::SerializableSubject<bool>             isReinforceMode_;
        std::queue<std::unique_ptr<IDamage>>        onDamagedStack_;
        
        

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            archive(CEREAL_NVP(maxHealth_));
            archive(CEREAL_NVP(minHealth_));
            archive(CEREAL_NVP(health_));
            archive(CEREAL_NVP(maxEnhancePowerStack_));
            archive(CEREAL_NVP(enhancePowerStack_));
            archive(CEREAL_NVP(dashAttack_));
            archive(CEREAL_NVP(walkSpeed_));
            archive(CEREAL_NVP(runSpeed_));
            archive(CEREAL_NVP(moveRotateSpeed_));
            archive(CEREAL_NVP(jumpPower_));
            archive(CEREAL_NVP(onEnableReinforceDuration_secs_));
            archive(CEREAL_NVP(onDisableReinforceDuration_secs_));
            archive(CEREAL_NVP(reinforceRequireEnhance_));
            archive(CEREAL_NVP(damageStateDuration_secs_));
            archive(CEREAL_NVP(deathStateDuration_secs_));
            archive(CEREAL_NVP(reinforceModeDuring_secs_));
            archive(CEREAL_NVP(reinforceModeDuration_secs_));
            archive(CEREAL_NVP(quests_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IPlayerAvatarStatus>(this));
            if (version >= 0) archive(CEREAL_NVP(maxHealth_));
            if (version >= 0) archive(CEREAL_NVP(minHealth_));
            if (version >= 0) archive(CEREAL_NVP(health_));
            if (version >= 0) archive(CEREAL_NVP(maxEnhancePowerStack_));
            if (version >= 0) archive(CEREAL_NVP(enhancePowerStack_));
            if (version >= 0) archive(CEREAL_NVP(dashAttack_));
            if (version >= 0) archive(CEREAL_NVP(walkSpeed_));
            if (version >= 0) archive(CEREAL_NVP(runSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveRotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(jumpPower_));
            if (version >= 0) archive(CEREAL_NVP(onEnableReinforceDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(onDisableReinforceDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(reinforceRequireEnhance_));
            if (version >= 0) archive(CEREAL_NVP(damageStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(deathStateDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(reinforceModeDuring_secs_));
            if (version >= 0) archive(CEREAL_NVP(reinforceModeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(quests_));
        }
    };
#pragma endregion 
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::IPlayerAvatarStatus, GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus);
#pragma endregion