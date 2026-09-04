#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "vec3.hpp"
#include "../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"

namespace GameCore::PathFinding
{
    /**
     * HeightGridMap 上の A* 経路探索をバックグラウンドスレッドで実行するクラス。
     * Tick() を毎フレーム呼ぶと searchIntervalSec 間隔でバックグラウンド検索を起動し、
     * 結果が出たら Path() で取得できる。
     */
    class HeightGridAstar
    {
    public:
        HeightGridAstar()  = default;
        ~HeightGridAstar();

        /** 検索完了時にキャッシュを更新し、必要なら新しい検索を起動する */
        void Tick(
            const std::shared_ptr<NanamiEngine::Module::Asset::HeightGridMap>& grid,
            const glm::vec3& start,
            const std::vector<glm::vec3>& goals,
            int maxCellRange, float maxClimbAngleDeg, float searchIntervalSec);

        std::vector<glm::vec3>&       Path()       { return cachedPath_; }
        const std::vector<glm::vec3>& Path() const { return cachedPath_; }
        bool HasPath()  const { return hasPath_; }
        void ClearPath()      { hasPath_ = false; searchTimer_ = 0.0f; } // 即座に再探索を起動させる

    private:
        static std::vector<glm::vec3> FindPath(
            const NanamiEngine::Module::Asset::HeightGridMap& grid,
            const glm::vec3& start, const glm::vec3& goal,
            int maxCellRange, float maxClimbAngleDeg);

        static constexpr int kDirX[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
        static constexpr int kDirZ[8] = {  0,  0,  1, -1,  1, -1,  1, -1 };

        std::atomic_bool       isSearching_{false};
        std::atomic_bool       isReady_{false};
        bool                   hasPath_     = false;
        float                  searchTimer_ = 0.0f;
        std::vector<glm::vec3> cachedPath_;
        std::vector<glm::vec3> resultPath_;
        std::mutex             mutex_;
        std::thread            pathThread_;
    };
}
