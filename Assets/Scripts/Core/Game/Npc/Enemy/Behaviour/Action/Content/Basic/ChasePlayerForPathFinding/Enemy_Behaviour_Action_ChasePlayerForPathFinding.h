#pragma once
#include "../../../../../../../PathFinding/HeightGridAstar/Multithread/PathFinding_HeightGridAstar_Multithread.h"

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** HeightGridMap の格子グリッドで PathFinder を使って経路探索をワーカースレッドで実行し、プレイヤーへ向かうアクション。
     * NOTE:
     * - 1フレーム目: 探索スレッドを起動するだけで移動せず Failure を返す。
     * - 2フレーム目以降: 前フレームまでに計算された経路で移動する。
     *     移動できれば SetLinearVelocity で移動して Success、できなければ Failure。
     * 探索は移動するたびに現在位置を始点として追跡し直す。
     */
    class ChasePlayerForPathFinding final : public ActionBase
    {
    public:
        ChasePlayerForPathFinding()           = default;
        ~ChasePlayerForPathFinding() override = default;

    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::HeightGridMap) heightGridMap_;
        [[serialize(0)]] float moveSpeed_           = 3.0f;   
        [[serialize(0)]] float rotateSpeed_         = 360.0f; 
        [[serialize(0)]] int   maxPathCellRange_    = 16;
        [[serialize(0)]] float maxClimbAngleDeg_    = 45.0f;
        [[serialize(0)]] float searchIntervalSec_   = 1.0f;
        [[serialize(0)]] int   animationNumber_     = -1; 
        [[serialize(0)]] float rotateToleranceDeg_  = 5.0f;

        PathFinding::HeightGridAstar pathFinder_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(heightGridMap_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(maxPathCellRange_));
            archive(CEREAL_NVP(maxClimbAngleDeg_));
            archive(CEREAL_NVP(searchIntervalSec_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(rotateToleranceDeg_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(heightGridMap_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(maxPathCellRange_));
            if (version >= 0) archive(CEREAL_NVP(maxClimbAngleDeg_));
            if (version >= 2) archive(CEREAL_NVP(searchIntervalSec_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
            if (version >= 1) archive(CEREAL_NVP(rotateToleranceDeg_));
        }
#pragma endregion

    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ChasePlayerForPathFinding, "Basic::ChasePlayerForPathFinding")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding, 2)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding
)
