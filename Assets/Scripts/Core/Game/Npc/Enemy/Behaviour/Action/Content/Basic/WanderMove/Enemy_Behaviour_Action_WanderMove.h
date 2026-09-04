#pragma once
#include "../../../../../../../PathFinding/HeightGridAstar/Multithread/PathFinding_HeightGridAstar_Multithread.h"

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

#include <random>

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** HeightGridMap の格子グリッドで PathFinder を使い、スポーン地点周辺をランダムに徘徊するアクション。
     * NOTE:
     * - Moving: PathFinding で目標地点へ移動する。到達したらIdle。
     * - Idle: waitTimeMin_, waitTimeMax_秒待機した後、新しい目標地点を選んでMoving。
     */
    class WanderMove final : public ActionBase
    {
    public:
        WanderMove()           = default;
        ~WanderMove() override = default;

    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::HeightGridMap) heightGridMap_;
        [[serialize(0)]] float wanderRadius_        = 10.0f;
        [[serialize(0)]] float waitTimeMin_         = 1.0f;
        [[serialize(0)]] float waitTimeMax_         = 3.0f;
        [[serialize(0)]] float moveSpeed_           = 2.0f;
        [[serialize(0)]] float rotateSpeed_         = 360.0f;
        [[serialize(0)]] float rotateToleranceDeg_  = 5.0f;
        [[serialize(0)]] int   maxPathCellRange_    = 16;
        [[serialize(0)]] float maxClimbAngleDeg_    = 45.0f;
        [[serialize(0)]] float searchIntervalSec_   = 1.0f;
        [[serialize(0)]] int   animationMoveNumber_ = -1;
        [[serialize(0)]] int   animationIdleNumber_ = -1;

        enum class State { Idle, Moving };
        State     state_               = State::Idle;
        bool      spawnPosInitialized_ = false;
        glm::vec3 spawnPos_            = {};
        glm::vec3 wanderTarget_        = {};
        float     waitTimer_           = 0.0f;
        float     currentWaitTime_     = 0.0f;

        PathFinding::HeightGridAstar pathFinder_;
        std::mt19937                 rng_{ std::random_device{}() };

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(heightGridMap_));
            archive(CEREAL_NVP(wanderRadius_));
            archive(CEREAL_NVP(waitTimeMin_));
            archive(CEREAL_NVP(waitTimeMax_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(rotateToleranceDeg_));
            archive(CEREAL_NVP(maxPathCellRange_));
            archive(CEREAL_NVP(maxClimbAngleDeg_));
            archive(CEREAL_NVP(searchIntervalSec_));
            archive(CEREAL_NVP(animationMoveNumber_));
            archive(CEREAL_NVP(animationIdleNumber_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(heightGridMap_));
            if (version >= 0) archive(CEREAL_NVP(wanderRadius_));
            if (version >= 0) archive(CEREAL_NVP(waitTimeMin_));
            if (version >= 0) archive(CEREAL_NVP(waitTimeMax_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateToleranceDeg_));
            if (version >= 0) archive(CEREAL_NVP(maxPathCellRange_));
            if (version >= 0) archive(CEREAL_NVP(maxClimbAngleDeg_));
            if (version >= 0) archive(CEREAL_NVP(searchIntervalSec_));
            if (version >= 0) archive(CEREAL_NVP(animationMoveNumber_));
            if (version >= 0) archive(CEREAL_NVP(animationIdleNumber_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(WanderMove, "Basic::WanderMove")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::WanderMove, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::WanderMove)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::WanderMove
)
