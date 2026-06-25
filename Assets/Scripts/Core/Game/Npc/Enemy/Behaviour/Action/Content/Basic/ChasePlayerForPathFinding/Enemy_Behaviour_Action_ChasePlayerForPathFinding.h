#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "vec3.hpp"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** HeightGridMapの格子情報からX軸の登降可否を判定し、A* 経路探索を別スレッドで実行してプレイヤーへ向かうアクション。
     * NOTE:
     * - 1フレーム目: 探索スレッドを起動するだけで移動せず Failure を返す。
     * - 2フレーム目以降: 前フレームまでに完了した経路で移動する。
     *     移動できれば SetLinearVelocity で移動して Success、できなければ Failure。
     * 探索は完了するたびに現在位置を始点として追跡し直す。
     */
    class ChasePlayerForPathFinding final : public ActionBase
    {
    public:
        ChasePlayerForPathFinding() = default;
        ~ChasePlayerForPathFinding() override;

    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        // ワーカースレッドで実行される経路探索本体。grid と位置のみを参照する純粋な関数。
        // 始点セルから目的地まで進むべきセルすべてのワールド座標を返す。
        static std::vector<glm::vec3> FindPath(
            const Asset::HeightGridMap& grid,
            const glm::vec3& start, const glm::vec3& goal,
            int maxCellRange, float maxClimbAngleDeg);

        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::HeightGridMap) heightGridMap_;
        [[serialize(0)]] float moveSpeed_           = 3.0f;  // 移動速度
        [[serialize(0)]] float rotateSpeed_         = 360.0f;// 移動方向への正面Y軸回転速度(度/秒)
        [[serialize(0)]] int   maxPathCellRange_    = 16;    // 探索するセルの最大範囲
        [[serialize(0)]] float maxClimbAngleDeg_    = 45.0f; // 越えられる最大傾斜角(度)
        [[serialize(0)]] float searchIntervalSec_   = 1.0f;  // 経路探索を走らせる間隔(秒)
        [[serialize(0)]] int   animationNumber_     = -1;    // 移動時に再生するアニメーション番号(-1で無効)

        [[serialize(0)]] float rotateToleranceDeg_ = 5.0f;  // 移動方向の角度(度)以下なら回転しない(無駄な回転防止)

        std::atomic_bool       isSearching_{false}; // 探索スレッドが実行中か
        std::atomic_bool       isReady_{false};     // 新しい探索結果を利用可能か
        bool                   hasPath_ = false;    // 移動に使える経路を保持しているか
        float                  searchTimer_ = 0.0f; // 次の探索を起動するまでの残り時間(秒)
        std::vector<glm::vec3> cachedPath_;         // メインスレッドが移動に使う経路
        std::vector<glm::vec3> resultPath_;         // ワーカースレッドが格納した経路
        std::mutex             mutex_;
        std::thread            pathThread_;

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
