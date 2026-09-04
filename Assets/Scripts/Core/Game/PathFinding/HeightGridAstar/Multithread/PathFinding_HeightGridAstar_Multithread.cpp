#include "PathFinding_HeightGridAstar_Multithread.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "../../../../../../../Engine/Core/Application/Time/Time.h"

namespace GameCore::PathFinding
{
    constexpr float PI    = 3.14159265358979323846f;
    constexpr float SQRT2 = 1.41421356237309504880f;

    HeightGridAstar::~HeightGridAstar()
    {
        if (pathThread_.joinable())
            pathThread_.join();
    }

    void HeightGridAstar::Tick(
        const std::shared_ptr<NanamiEngine::Module::Asset::HeightGridMap>& grid,
        const glm::vec3& start,
        const std::vector<glm::vec3>& goals,
        const int maxCellRange, const float maxClimbAngleDeg, const float searchIntervalSec)
    {
        if (isReady_.load(std::memory_order_acquire))
        {
            {
                std::lock_guard lock(mutex_);
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
                if (pathThread_.joinable())
                    pathThread_.join();

                const auto  gridShared = grid;
                const auto  goalsCopy  = goals;
                const int   range      = maxCellRange;
                const float angle      = maxClimbAngleDeg;
                pathThread_ = std::thread([this, gridShared, start, goalsCopy, range, angle]()
                {
                    std::vector<glm::vec3> bestPath;
                    for (const auto& goalPos : goalsCopy)
                    {
                        auto path = FindPath(*gridShared, start, goalPos, range, angle);
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

    std::vector<glm::vec3> HeightGridAstar::FindPath(
        const NanamiEngine::Module::Asset::HeightGridMap& grid,
        const glm::vec3& start, const glm::vec3& goal,
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

        const size_t cellCount = static_cast<size_t>(W) * static_cast<size_t>(H);
        std::vector gScore  (cellCount, std::numeric_limits<float>::max());
        std::vector cameFrom(cellCount, -1);
        std::vector closed  (cellCount, false);

        struct Node { float f; int x; int z; };
        const auto cmp = [](const Node& a, const Node& b) { return a.f > b.f; };
        std::priority_queue<Node, std::vector<Node>, decltype(cmp)> open(cmp);

        gScore[index(sx, sz)] = 0.0f;
        open.push({heuristic(sx, sz), sx, sz});

        bool reached = false;
        while (!open.empty())
        {
            const Node current = open.top();
            open.pop();

            const int ci = index(current.x, current.z);
            if (closed[ci]) continue;
            closed[ci] = true;

            if (current.x == gx && current.z == gz)
            {
                reached = true;
                break;
            }

            const float curHeight = grid.At(current.x, current.z).height;

            for (int d = 0; d < 8; ++d)
            {
                const int nx = current.x + kDirX[d];
                const int nz = current.z + kDirZ[d];

                if (!inBounds(nx, nz))
                    continue;
                if (std::abs(nx - sx) > maxCellRange || std::abs(nz - sz) > maxCellRange)
                    continue;

                const bool  diagonal = (kDirX[d] != 0 && kDirZ[d] != 0);
                const float horiz    = diagonal ? orthoDist * SQRT2 : orthoDist;
                if (horiz <= 0.0f)
                    continue;

                const float nHeight = grid.At(nx, nz).height;
                if (std::abs(nHeight - curHeight) > maxSlopeTan * horiz)
                    continue;

                const int ni = index(nx, nz);
                if (closed[ni])
                    continue;

                const float tentative = gScore[ci] + horiz;
                if (tentative < gScore[ni])
                {
                    gScore[ni]   = tentative;
                    cameFrom[ni] = ci;
                    open.push({tentative + heuristic(nx, nz), nx, nz});
                }
            }
        }

        if (!reached)
            return {};

        std::vector<glm::vec3> path;
        const int startIdx = index(sx, sz);
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
