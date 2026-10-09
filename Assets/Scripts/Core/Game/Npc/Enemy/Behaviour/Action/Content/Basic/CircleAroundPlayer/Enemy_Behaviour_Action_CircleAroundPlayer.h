#pragma once
#include <random>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    // NOTE: 一番近いプレイヤーの周りを desiredRadius_ を保って回る。向きと時間は毎回ランダム
    // NOTE: 進めなければ一度だけ反転し、それでも駄目なら Success。radiusShrinkPerSec_ > 0 なら minRadius_ まで詰め寄る
    class CircleAroundPlayer final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoReset() override;
        void       DoDrawGui() override;

        [[serialize(0)]] float moveSpeed_       = 15.0f;
        [[serialize(0)]] float rotateSpeed_     = 180.0f;
        [[serialize(0)]] float minSeconds_      = 1.5f;
        [[serialize(0)]] float maxSeconds_      = 3.0f;
        [[serialize(0)]] float desiredRadius_   = 10.0f;
        [[serialize(0)]] float radiusGain_      = 1.0f;
        [[serialize(0)]] int   animationNumber_ = -1;
        [[serialize(1)]] float radiusShrinkPerSec_ = 0.0f;
        [[serialize(1)]] float minRadius_          = 0.0f;
        // NOTE: 実際の移動量が期待値のこの割合を下回る状態が stuckThreshold_secs_ 続いたら詰まりとみなす
        [[serialize(2)]] float stuckProgressRate_  = 0.2f;
        [[serialize(2)]] float stuckThreshold_secs_ = 0.3f;

        bool         isRunning_      = false;
        float        direction_      = 1.0f;
        float        during_secs_    = 0.0f;
        float        duration_secs_  = 0.0f;
        float        stuck_secs_     = 0.0f;
        bool         hasFlipped_     = false;
        float        lastTickTime_   = 0.0f;
        glm::vec3    lastPosition_   = glm::vec3(0.0f);
        std::mt19937 rng_{ std::random_device{}() };

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(minSeconds_));
            archive(CEREAL_NVP(maxSeconds_));
            archive(CEREAL_NVP(desiredRadius_));
            archive(CEREAL_NVP(radiusGain_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(radiusShrinkPerSec_));
            archive(CEREAL_NVP(minRadius_));
            archive(CEREAL_NVP(stuckProgressRate_));
            archive(CEREAL_NVP(stuckThreshold_secs_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(minSeconds_));
            if (version >= 0) archive(CEREAL_NVP(maxSeconds_));
            if (version >= 0) archive(CEREAL_NVP(desiredRadius_));
            if (version >= 0) archive(CEREAL_NVP(radiusGain_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
            if (version >= 1) archive(CEREAL_NVP(radiusShrinkPerSec_));
            if (version >= 1) archive(CEREAL_NVP(minRadius_));
            if (version >= 2) archive(CEREAL_NVP(stuckProgressRate_));
            if (version >= 2) archive(CEREAL_NVP(stuckThreshold_secs_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(CircleAroundPlayer, "Basic::CircleAroundPlayer")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::CircleAroundPlayer, 2)
