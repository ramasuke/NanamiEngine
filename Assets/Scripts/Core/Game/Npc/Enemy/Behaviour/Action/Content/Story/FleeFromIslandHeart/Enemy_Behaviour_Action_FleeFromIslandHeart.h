#pragma once
#include <random>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    // NOTE: 島の心臓が抜けたら心臓の方を振り向いてから反対へ走り去り、離れたら消える。抜けるまでは Failure
    class FleeFromIslandHeart final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        void Face(const TickContext& context, const glm::vec3& direction) const;
        static void Leave(const TickContext& context);

        // NOTE: 振り向くまでの間 (頭数ぶんばらけさせる)
        [[serialize(0)]] float reactMin_secs_       = 0.2f;
        [[serialize(0)]] float reactMax_secs_       = 1.2f;
        // NOTE: 心臓の方を向いて立ち止まる長さ
        [[serialize(0)]] float startle_secs_        = 1.0f;
        [[serialize(0)]] float moveSpeed_           = 8.0f;
        [[serialize(0)]] float rotateSpeed_         = 540.0f;
        // NOTE: 心臓から真っ直ぐ離れる向きを、左右へこの角度までばらけさせる
        [[serialize(0)]] float fleeSpreadDegrees_   = 35.0f;
        // NOTE: 最寄りのプレイヤーからこれだけ離れたら消える
        [[serialize(0)]] float vanishDistance_      = 45.0f;
        // NOTE: 離れきれなくても、走りはじめてからこれだけ経ったら消える
        [[serialize(0)]] float maxFlee_secs_        = 8.0f;
        [[serialize(0)]] int   animationStartleNumber_ = -1;
        [[serialize(0)]] int   animationMoveNumber_    = -1;

        enum class State { Waiting, Reacting, Startled, Fleeing };
        State     state_          = State::Waiting;
        float     timer_secs_     = 0.0f;
        float     react_secs_     = 0.0f;
        glm::vec3 fleeDirection_  = {};
        std::mt19937 rng_{ std::random_device{}() };

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(reactMin_secs_));
            archive(CEREAL_NVP(reactMax_secs_));
            archive(CEREAL_NVP(startle_secs_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(fleeSpreadDegrees_));
            archive(CEREAL_NVP(vanishDistance_));
            archive(CEREAL_NVP(maxFlee_secs_));
            archive(CEREAL_NVP(animationStartleNumber_));
            archive(CEREAL_NVP(animationMoveNumber_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(reactMin_secs_));
            if (version >= 0) archive(CEREAL_NVP(reactMax_secs_));
            if (version >= 0) archive(CEREAL_NVP(startle_secs_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(fleeSpreadDegrees_));
            if (version >= 0) archive(CEREAL_NVP(vanishDistance_));
            if (version >= 0) archive(CEREAL_NVP(maxFlee_secs_));
            if (version >= 0) archive(CEREAL_NVP(animationStartleNumber_));
            if (version >= 0) archive(CEREAL_NVP(animationMoveNumber_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(FleeFromIslandHeart, "Story::FleeFromIslandHeart")
}
