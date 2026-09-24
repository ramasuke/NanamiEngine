#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../Other/WriteBlackBoard/Enemy_Behaviour_Action_WriteBlackBoardInt.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GamePlay::Prop
{
    class ChargeBreakPillar;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief プレイヤーへ向き直ってから一直線に突進し、ChargeStuckObstacle に当たったら頭が刺さったことを黒板に書く
     * @note 刺さった先が ChargeBreakPillar なら柱を倒し、柱のダメージを自分に入れる
     * @note aimAtIntroPillar_ ならプレイヤーではなく登場演出用の柱へ向き直り、プレイヤーには当てない
     */
    class ChargeRush final : public ActionBase
    {
        enum class Phase
        {
            WindUp,
            Rush,
            Impact,
        };

        enum class CastResult
        {
            None,
            Wall,
            StuckObstacle,
        };

        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;
        void       DoReset() override;

        void RotateToPlayer(const TickContext& context) const;
        void RotateTowards (const TickContext& context, const glm::vec3& targetPos) const;
        void TryHitPlayer  (const TickContext& context);
        // NOTE: 自分の体と地面(上向きの面)は当たっていないものとして扱う
        // NOTE: 刺さった先が倒せる柱なら stuckPillar_ に残す
        [[nodiscard]] CastResult CastForward(const TickContext& context);
        void PlayImpactSound(const TickContext& context) const;
        void CollapsePillar (const TickContext& context) const;
        TickStatus Finish(const TickContext& context);

        [[serialize(0)]] float       windUp_secs_           = 0.6f;
        [[serialize(0)]] float       rotateSpeed_           = 180.0f;
        [[serialize(0)]] float       moveSpeed_             = 90.0f;
        [[serialize(0)]] float       maxRush_secs_          = 2.5f;
        [[serialize(0)]] int         windUpAnimationNumber_ = -1;
        [[serialize(0)]] int         rushAnimationNumber_   = 58;
        [[serialize(0)]] int         impactAnimationNumber_ = 41;
        [[serialize(0)]] float       impact_secs_           = 1.0f;
        [[serialize(0)]] glm::vec3   castOriginOffset_      = glm::vec3(0.0f, 25.0f, -40.0f);
        [[serialize(0)]] float       castRadius_            = 8.0f;
        [[serialize(0)]] float       castDistance_          = 6.0f;
        // NOTE: 地面や坂を壁と見なさないための、法線の上向き成分の上限
        [[serialize(0)]] float       wallMaxNormalY_        = 0.7f;
        [[serialize(0)]] std::string attackAreaName_        = "HeadButt";
        [[serialize(0)]] Damage::PhysicsPower attackPower_;
        [[serialize(0)]] std::string stuckStateKeyName_     = "StuckState";
        [[serialize(0)]] FIELD(Asset::SoundFile) impactSound_;
        [[serialize(0)]] WriteBlackBoard finishedWriteBlackBoard_ = WriteBlackBoard();
        [[serialize(1)]] bool        aimAtIntroPillar_      = false;

        Phase     phase_         = Phase::WindUp;
        float     during_secs_   = 0.0f;
        glm::vec3 rushDirection_ = glm::vec3(0.0f, 0.0f, -1.0f);
        bool      isAttacked_    = false;
        bool      isStuck_       = false;
        std::weak_ptr<GamePlay::Prop::ChargeBreakPillar> stuckPillar_;
        std::weak_ptr<GamePlay::Prop::ChargeBreakPillar> introPillar_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(windUp_secs_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(maxRush_secs_));
            archive(CEREAL_NVP(windUpAnimationNumber_));
            archive(CEREAL_NVP(rushAnimationNumber_));
            archive(CEREAL_NVP(impactAnimationNumber_));
            archive(CEREAL_NVP(impact_secs_));
            archive(CEREAL_NVP(castOriginOffset_));
            archive(CEREAL_NVP(castRadius_));
            archive(CEREAL_NVP(castDistance_));
            archive(CEREAL_NVP(wallMaxNormalY_));
            archive(CEREAL_NVP(attackAreaName_));
            archive(CEREAL_NVP(attackPower_));
            archive(CEREAL_NVP(stuckStateKeyName_));
            archive(CEREAL_NVP(impactSound_));
            archive(CEREAL_NVP(finishedWriteBlackBoard_));
            archive(CEREAL_NVP(aimAtIntroPillar_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(windUp_secs_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(maxRush_secs_));
            if (version >= 0) archive(CEREAL_NVP(windUpAnimationNumber_));
            if (version >= 0) archive(CEREAL_NVP(rushAnimationNumber_));
            if (version >= 0) archive(CEREAL_NVP(impactAnimationNumber_));
            if (version >= 0) archive(CEREAL_NVP(impact_secs_));
            if (version >= 0) archive(CEREAL_NVP(castOriginOffset_));
            if (version >= 0) archive(CEREAL_NVP(castRadius_));
            if (version >= 0) archive(CEREAL_NVP(castDistance_));
            if (version >= 0) archive(CEREAL_NVP(wallMaxNormalY_));
            if (version >= 0) archive(CEREAL_NVP(attackAreaName_));
            if (version >= 0) archive(CEREAL_NVP(attackPower_));
            if (version >= 0) archive(CEREAL_NVP(stuckStateKeyName_));
            if (version >= 0) archive(CEREAL_NVP(impactSound_));
            if (version >= 0) archive(CEREAL_NVP(finishedWriteBlackBoard_));
            if (version >= 1) archive(CEREAL_NVP(aimAtIntroPillar_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ChargeRush, "Tyrannosaurus::ChargeRush")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ChargeRush, 1);
