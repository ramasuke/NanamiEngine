#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

#include "vec2.hpp"
#include "vec3.hpp"
#include "../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"

namespace GameCore::PathFinding
{
    /**
     * searchIntervalSec 間隔でバックグラウンド検索し、結果を Path() で返す
     * @note 探索ループでは STL イテレータを作らない (Debug の MSVC STL はイテレータ毎に共通ロックを取り極端に遅い)
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
            std::span<const glm::ivec2> directions,
            int maxCellRange, float maxClimbAngleDeg, float searchIntervalSec);

        std::vector<glm::vec3>&       Path()       { return cachedPath_; }
        const std::vector<glm::vec3>& Path() const { return cachedPath_; }
        bool HasPath()  const { return hasPath_; }
        void ClearPath()      { hasPath_ = false; searchTimer_ = 0.0f; } // 即座に再探索を起動させる

    private:
        /** open リストの要素。f = 始点からのコスト + ゴールまでの推定距離 */
        struct OpenNode
        {
            float f;
            int   x;
            int   z;
        };

        struct SearchScratch
        {
            std::vector<float>         gScore;       // 始点からの最小コスト。openStamp が今回のときだけ有効
            std::vector<int>           cameFrom;     // 経路を逆にたどるための直前セル。openStamp が今回のときだけ有効
            std::vector<std::uint32_t> openStamp;    // gScore / cameFrom を今回の探索で書いたか
            std::vector<std::uint32_t> closedStamp;  // 今回の探索で確定済みか
            std::vector<OpenNode>      heap;         // open リストの二分ヒープ(先頭 heapSize 個が有効)
            std::uint32_t              searchStamp = 0;
        };
        
        static void     HeapPush(OpenNode* heap, std::size_t& size, const OpenNode& node);
        static OpenNode HeapPop (OpenNode* heap, std::size_t& size);

        static std::vector<glm::vec3> FindPath(
            const NanamiEngine::Module::Asset::HeightGridMap& grid,
            SearchScratch& scratch,
            const glm::vec3& start, const glm::vec3& goal,
            std::span<const glm::ivec2> directions,
            int maxCellRange, float maxClimbAngleDeg);

        std::atomic_bool       isSearching_{false};
        std::atomic_bool       isReady_{false};
        bool                   hasPath_     = false;
        float                  searchTimer_ = 0.0f;
        std::vector<glm::vec3> cachedPath_;
        std::vector<glm::vec3> resultPath_;
        std::mutex             mutex_;
        std::thread            pathThread_;
        SearchScratch          scratch_;
    };
}
