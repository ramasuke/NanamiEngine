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
     * Tick() を毎フレーム呼ぶと searchIntervalSec間隔でバックグラウンド検索を起動し、
     * 結果が出たら Path() で取得できる。
     *
     * @note 探索の内側のループでは STL のイテレータを一切作らない(作業用の配列は生ポインタで触る)。
     *       Debug ビルドの MSVC STL はイテレータの生成・破棄のたびにプロセス共通のロック(std::_Lockit)を取るため、
     *       複数の敵が同時に探索するとこのロックを奪い合い、同じロックを使うメインスレッドの文字列や vector の
     *       操作まで待たされて FPS が 1 桁まで落ちていた(ハイエナ 9 頭で 60 → 6〜15)。
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

        /**
         * 探索の作業用配列。セル数ぶん(750x750 なら約 56 万)あるので、探索のたびに確保・初期化せず
         * このインスタンスで使い回す。探索中はワーカースレッドだけが触る(次の探索は前のスレッドを join してから起動する)。
         *
         * 毎回全セルを初期化しなくて済むよう、セルごとに「何回目の探索で書いたか」を stamp で持つ。
         * stamp が今回の searchStamp と違うセルは「未訪問」として扱う。
         */
        struct SearchScratch
        {
            std::vector<float>         gScore;       // 始点からの最小コスト。openStamp が今回のときだけ有効
            std::vector<int>           cameFrom;     // 経路を逆にたどるための直前セル。openStamp が今回のときだけ有効
            std::vector<std::uint32_t> openStamp;    // gScore / cameFrom を今回の探索で書いたか
            std::vector<std::uint32_t> closedStamp;  // 今回の探索で確定済みか
            std::vector<OpenNode>      heap;         // open リストの二分ヒープ(先頭 heapSize 個が有効)
            std::uint32_t              searchStamp = 0;
        };

        /**
         * open リストの二分ヒープ(f が最小のものが根)。heap の先頭 size 個が有効で、容量の確保は呼び出し側が行う。
         * std::priority_queue / std::push_heap は Debug ビルドでイテレータを作ってロックを取るため自前で持つ
         */
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
