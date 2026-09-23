#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../Other/WriteBlackBoard/Enemy_Behaviour_Action_WriteBlackBoardInt.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PhysicsAttack final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;
        void       UpdateMovement(const TickContext& context) const;

        [[serialize(0)]] std::string attackAreaName_;
        [[serialize(0)]] Damage::PhysicsPower attackPower_;
        [[serialize(1)]] float normalAttackOccurrenceDuration_secs_ = 0.0f;
        [[serialize(1)]] float normalAttackDuration_secs_ = 0.0f;
        [[serialize(2)]] int animationNumber_ = -1;
        [[serialize(4)]] FIELD(Asset::SoundFile) attackSound_;
        // trackEnd_secs_ まで一番近いプレイヤーへ向き直る(0 で無効)
        [[serialize(5)]] float trackRotateSpeed_  = 0.0f;
        [[serialize(5)]] float trackEnd_secs_     = 0.0f;
        // [lungeStart_secs_, lungeEnd_secs_] の間だけ前へ踏み込む(0 で無効)。有効時は区間外の水平速度を 0 にする
        [[serialize(5)]] float lungeSpeed_        = 0.0f;
        [[serialize(5)]] float lungeStart_secs_   = 0.0f;
        [[serialize(5)]] float lungeEnd_secs_     = 0.0f;
        [[serialize(5)]] float lungeStopDistance_ = 0.0f;
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
            archive(CEREAL_NVP(trackRotateSpeed_));
            archive(CEREAL_NVP(trackEnd_secs_));
            archive(CEREAL_NVP(lungeSpeed_));
            archive(CEREAL_NVP(lungeStart_secs_));
            archive(CEREAL_NVP(lungeEnd_secs_));
            archive(CEREAL_NVP(lungeStopDistance_));
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
            if (version >= 5) archive(CEREAL_NVP(trackRotateSpeed_));
            if (version >= 5) archive(CEREAL_NVP(trackEnd_secs_));
            if (version >= 5) archive(CEREAL_NVP(lungeSpeed_));
            if (version >= 5) archive(CEREAL_NVP(lungeStart_secs_));
            if (version >= 5) archive(CEREAL_NVP(lungeEnd_secs_));
            if (version >= 5) archive(CEREAL_NVP(lungeStopDistance_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(PhysicsAttack, "EnemyStatus::PhysicsAttack")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PhysicsAttack, 5)
