#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../DamageContext/Physics/PhysicsPower.h"
#include "../../Other/WriteBlackBoard/Enemy_Behaviour_Action_WriteBlackBoard.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PhysicsAttack final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] std::string attackAreaName_;
        [[serialize(0)]] Damage::PhysicsPower attackPower_;
        [[serialize(1)]] float normalAttackOccurrenceDuration_secs_ = 0.0f;
        [[serialize(1)]] float normalAttackDuration_secs_ = 0.0f;
        [[serialize(2)]] int animationNumber_ = -1;
        [[serialize(4)]] FIELD(Asset::SoundFile) attackSound_;
        WriteBlackBoard finishedAttackWriteBlackBoard_ = WriteBlackBoard();
        float during_secs_ = 0.0f;
        bool  isAttacked_  = false;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(attackAreaName_));
            archive(CEREAL_NVP(attackPower_));
            archive(CEREAL_NVP(normalAttackOccurrenceDuration_secs_));
            archive(CEREAL_NVP(normalAttackDuration_secs_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(finishedAttackWriteBlackBoard_));
            archive(CEREAL_NVP(attackSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(attackAreaName_));
            if (version >= 0) archive(CEREAL_NVP(attackPower_));
            if (version >= 1) archive(CEREAL_NVP(normalAttackOccurrenceDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(normalAttackDuration_secs_));
            if (version >= 2) archive(CEREAL_NVP(animationNumber_));
            if (version >= 3) archive(CEREAL_NVP(finishedAttackWriteBlackBoard_));
            if (version >= 4) archive(CEREAL_NVP(attackSound_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(PhysicsAttack, "EnemyStatus::PhysicsAttack")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PhysicsAttack, 4)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PhysicsAttack)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PhysicsAttack)
