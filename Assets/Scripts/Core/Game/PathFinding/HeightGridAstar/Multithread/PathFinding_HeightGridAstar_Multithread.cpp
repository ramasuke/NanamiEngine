#include "PathFinding_HeightGridAstar_Multithread.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "Engine/Core/Application/Time/Time.h"

namespace GameCore::PathFinding
{
    constexpr float PI    = 3.14159265358979323846f;
    constexpr float SQRT2 = 1.41421356237309504880f;

    // open リストの最初の容量。足りなくなったら FindPath が倍々で広げる
    constexpr std::size_t HEIGHT_GRID_ASTAR_INITIAL_HEAP_CAPACITY = 4096;

    HeightGridAstar::~HeightGridAstar()
    {
        if (pathThread_.joinable())
            pathThread_.join();
    }

    void HeightGridAstar::Tick(
        const std::shared_ptr<NanamiEngine::Module::Asset::HeightGridMap>& grid,
        const glm::vec3& start,
        const std::vector<glm::vec3>& goals,
        const std::span<const glm::ivec2> directions,
        const int maxCellRange, const float maxClimbAngleDeg, const float searchIntervalSec)
    {
        if (isReady_.load(std::memory_order_acquire))
        {
            {
                std::scoped_lock lock(mutex_);
                cachedPath_ = resultPath_;
            }
            hasPath_     = !cachedPath_.empty();
            isReady_     = false;
            isSearching_ = false;
        }

        if (!isSearching_)
        {
            searchTimer_ -= NanamiEngine::Time::DeltaTime();
            if (searchTimer_ <= 0.0f)
            {
                searchTimer_ = (std::max)(0.0f, searchIntervalSec);
                isSearching_ = true;
                // 前回の探索スレッドは結果を書き終えている(isReady_ を見てから来る)ので、ここの join は待たない。
                // scratch_ は探索スレッドだけが触るため、join してから次のスレッドを起動すれば共有にならない
                if (pathThread_.joinable())
                    pathThread_.join();

                const auto  gridShared = grid;
                const auto  goalsCopy      = goals;
                const auto  directionsCopy = std::vector<glm::ivec2>(directions.begin(), directions.end());
                const int   range          = maxCellRange;
                const float angle          = maxClimbAngleDeg;
                pathThread_ = std::thread([this, gridShared, start, goalsCopy, directionsCopy, range, angle]()
                {
                    std::vector<glm::vec3> bestPath;
                    for (const auto& goalPos : goalsCopy)
                    {
                        auto path = FindPath(*gridShared, scratch_, start, goalPos, directionsCopy, range, angle);
                        if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size()))
                            bestPath = std::move(path);
                    }
                    {
                        std::lock_guard lock(mutex_);
                        resultPath_ = std::move(bestPath);
                    }
                    isReady_.store(true, std::memory_order_release);
                });
            }
        }
    }

    void HeightGridAstar::HeapPush(OpenNode* heap, std::size_t& size, const OpenNode& node)
    {
        // 末尾に置いて、親より f が小さい間は上へ入れ替える
        std::size_t i = size++;
        heap[i] = node;
        while (i > 0)
        {
            const std::size_t parent = (i - 1) / 2;
            if (heap[parent].f <= heap[i].f)
                break;
            std::swap(heap[parent], heap[i]);
            i = parent;
        }
    }

    HeightGridAstar::OpenNode HeightGridAstar::HeapPop(OpenNode* heap, std::size_t& size)
    {
        // 根(f 最小)を取り出し、末尾を根へ移して、子のうち小さい方より大きい間は下へ入れ替える
        const OpenNode top = heap[0];
        heap[0] = heap[--size];
        std::size_t i = 0;
        while (true)
        {
            const std::size_t left     = 2 * i + 1;
            const std::size_t right    = left + 1;
            std::size_t       smallest = i;
            if (left < size && heap[left].f < heap[smallest].f)
                smallest = left;
            if (right < size && heap[right].f < heap[smallest].f)
                smallest = right;
            if (smallest == i)
                break;
            std::swap(heap[i], heap[smallest]);
            i = smallest;
        }
        return top;
    }

    std::vector<glm::vec3> HeightGridAstar::FindPath(
        const NanamiEngine::Module::Asset::HeightGridMap& grid,
        SearchScratch& scratch,
        const glm::vec3& start, const glm::vec3& goal,
        const std::span<const glm::ivec2> directions,
        const int maxCellRange, const float maxClimbAngleDeg)
    {
        const int W = grid.DivisionsX();
        const int H = grid.DivisionsZ();
        if (W <= 0 || H <= 0) return {};

        const glm::vec2 cell        = grid.CellSize();
        const float     orthoDist   = (std::min)(std::abs(cell.x), std::abs(cell.y));
        const float     maxSlopeTan = std::tan(maxClimbAngleDeg * PI / 180.0f);

        const glm::vec3 gridOrigin = grid.CellToWorld(0, 0);
        const auto resolveCell = [&](const glm::vec3& worldPos, int& outX, int& outZ) -> bool
        {
            if (grid.WorldToCell(worldPos, outX, outZ)) return true;
            outX = std::clamp(static_cast<int>(std::round((worldPos.x - gridOrigin.x) / cell.x)), 0, W - 1);
            outZ = std::clamp(static_cast<int>(std::round((worldPos.z - gridOrigin.z) / cell.y)), 0, H - 1);
            return true;
        };

        int sx, sz, gx, gz;
        if (!resolveCell(start, sx, sz)) return {};
        if (!resolveCell(goal,  gx, gz)) return {};
        if (sx == gx && sz == gz) return {};

        const auto index    = [W](int x, int z) { return z * W + x; };
        const auto inBounds = [W, H](int x, int z) { return x >= 0 && x < W && z >= 0 && z < H; };
        const auto heuristic = [&](int x, int z)
        {
            const float dx = static_cast<float>(x - gx) * cell.x;
            const float dz = static_cast<float>(z - gz) * cell.y;
            return std::sqrt(dx * dx + dz * dz);
        };

        // 作業用配列はセル数が変わったとき(別の HeightGridMap を渡されたとき)だけ確保し直す
        const std::size_t cellCount = static_cast<std::size_t>(W) * static_cast<std::size_t>(H);
        if (scratch.gScore.size() != cellCount)
        {
            scratch.gScore     .assign(cellCount, 0.0f);
            scratch.cameFrom   .assign(cellCount, -1);
            scratch.openStamp  .assign(cellCount, 0u);
            scratch.closedStamp.assign(cellCount, 0u);
            scratch.searchStamp = 0;
        }
        if (scratch.heap.size() < HEIGHT_GRID_ASTAR_INITIAL_HEAP_CAPACITY)
            scratch.heap.resize(HEIGHT_GRID_ASTAR_INITIAL_HEAP_CAPACITY);

        // 今回の探索の番号。stamp が一周して 0 に戻ると、前の周で書いたセルと区別できなくなるので全部消してから 1 に戻す
        if (++scratch.searchStamp == 0)
        {
            std::fill_n(scratch.openStamp  .data(), cellCount, 0u);
            std::fill_n(scratch.closedStamp.data(), cellCount, 0u);
            scratch.searchStamp = 1;
        }
        const std::uint32_t stamp = scratch.searchStamp;

        // ここから先はイテレータを作らないよう、作業用配列も探索方向も生ポインタで触る
        float*         gScore       = scratch.gScore     .data();
        int*           cameFrom     = scratch.cameFrom   .data();
        std::uint32_t* openStamp    = scratch.openStamp  .data();
        std::uint32_t* closedStamp  = scratch.closedStamp.data();
        OpenNode*      heap         = scratch.heap       .data();
        std::size_t    heapCapacity = scratch.heap       .size();
        std::size_t    heapSize     = 0;
        const glm::ivec2* dirs     = directions.data();
        const std::size_t dirCount = directions.size();

        const int startIdx = index(sx, sz);
        gScore   [startIdx] = 0.0f;
        cameFrom [startIdx] = -1;
        openStamp[startIdx] = stamp;
        HeapPush(heap, heapSize, {heuristic(sx, sz), sx, sz});

        bool reached = false;
        while (heapSize > 0)
        {
            const OpenNode current = HeapPop(heap, heapSize);

            // 同じセルを安いコストで積み直すと古い方もヒープに残る。確定済みのセルが出てきたら読み飛ばす
            const int ci = index(current.x, current.z);
            if (closedStamp[ci] == stamp) continue;
            closedStamp[ci] = stamp;

            if (current.x == gx && current.z == gz)
            {
                reached = true;
                break;
            }

            const float curHeight = grid.At(current.x, current.z).height;
            const float curScore  = gScore[ci];

            for (std::size_t d = 0; d < dirCount; ++d)
            {
                const glm::ivec2 dir = dirs[d];
                const int nx = current.x + dir.x;
                const int nz = current.z + dir.y;

                if (!inBounds(nx, nz))
                    continue;
                if (std::abs(nx - sx) > maxCellRange || std::abs(nz - sz) > maxCellRange)
                    continue;

                const bool  diagonal = (dir.x != 0 && dir.y != 0);
                const float horiz    = diagonal ? orthoDist * SQRT2 : orthoDist;
                if (horiz <= 0.0f)
                    continue;

                const float nHeight = grid.At(nx, nz).height;
                if (std::abs(nHeight - curHeight) > maxSlopeTan * horiz)
                    continue;

                const int ni = index(nx, nz);
                if (closedStamp[ni] == stamp)
                    continue;

                // openStamp が今回でないセルは gScore が前の探索の値なので、未訪問(コスト無限大)として扱う
                const float tentative = curScore + horiz;
                if (openStamp[ni] != stamp || tentative < gScore[ni])
                {
                    gScore   [ni] = tentative;
                    cameFrom [ni] = ci;
                    openStamp[ni] = stamp;

                    // 容量が足りなくなったときだけ vector を広げる。ロックを取るのはここだけで、倍々なので回数はわずか
                    if (heapSize == heapCapacity)
                    {
                        scratch.heap.resize(heapCapacity * 2);
                        heap         = scratch.heap.data();
                        heapCapacity = scratch.heap.size();
                    }
                    HeapPush(heap, heapSize, {tentative + heuristic(nx, nz), nx, nz});
                }
            }
        }

        if (!reached)
            return {};

        std::vector<glm::vec3> path;
        for (int cur = index(gx, gz); cur != -1 && cur != startIdx; cur = cameFrom[cur])
        {
            const int cx = cur % W;
            const int cz = cur / W;
            path.push_back(grid.CellToWorld(cx, cz));
        }
        std::reverse(path.begin(), path.end());
        return path;
    }
}
