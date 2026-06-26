#include "Enemy_Behaviour_Action_ChasePlayerForPathFinding.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../glm/gtx/quaternion.hpp"
#include "../glm/gtx/vector_angle.hpp"

namespace GameCore::Npc::Enemy::Behaviour
{
    constexpr float PI    = 3.14159265358979323846f;
    constexpr float SQRT2 = 1.41421356237309504880f;

    Action::ChasePlayerForPathFinding::~ChasePlayerForPathFinding()
    {
        if (pathThread_.joinable())
            pathThread_.join();
    }

    bool Action::ChasePlayerForPathFinding::HasLineOfSight(
        const NanamiEngine::Module::Asset::HeightGridMap& grid,
        int x0, int z0, int x1, int z1,
        float maxSlopeTan, float orthoDist)
    {
        const int W = grid.DivisionsX();
        const int H = grid.DivisionsZ();

        int dx  = std::abs(x1 - x0);
        int dz  = std::abs(z1 - z0);
        int sx  = (x0 < x1) ? 1 : -1;
        int sz  = (z0 < z1) ? 1 : -1;
        int err = dx - dz;
        int cx  = x0, cz = z0;

        while (!(cx == x1 && cz == z1))
        {
            const int  prevX = cx;
            const int  prevZ = cz;
            const int  e2    = 2 * err;
            const bool stepX = (e2 > -dz);
            const bool stepZ = (e2 <  dx);
            if (stepX) { err -= dz; cx += sx; }
            if (stepZ) { err += dx; cz += sz; }

            if (cx < 0 || cx >= W || cz < 0 || cz >= H) return false;

            const float horiz = (stepX && stepZ) ? orthoDist * SQRT2 : orthoDist;
            if (std::abs(grid.At(cx, cz).height - grid.At(prevX, prevZ).height) > maxSlopeTan * horiz)
                return false;
        }
        return true;
    }

    std::vector<glm::vec3> Action::ChasePlayerForPathFinding::FindPath(
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

        // グリッド外の座標を最近傍セルにクランプして解決する
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

                // Theta*: 祖父ノードから nx,nz への LoS があれば祖父を親にする
                const int parentCi = cameFrom[ci];
                if (parentCi != -1)
                {
                    const int   px            = parentCi % W;
                    const int   pz            = parentCi / W;
                    if (HasLineOfSight(grid, px, pz, nx, nz, maxSlopeTan, orthoDist))
                    {
                        const float pdx           = static_cast<float>(nx - px) * cell.x;
                        const float pdz           = static_cast<float>(nz - pz) * cell.y;
                        const float tentativeViaP = gScore[parentCi] + std::sqrt(pdx * pdx + pdz * pdz);
                        if (tentativeViaP < gScore[ni])
                        {
                            gScore[ni]   = tentativeViaP;
                            cameFrom[ni] = parentCi;
                            open.push({tentativeViaP + heuristic(nx, nz), nx, nz});
                        }
                        continue; // LoS ルートで判断済みなので通常 A* は試みない
                    }
                }

                // 通常 A*: 現在ノード経由
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

        // ?o?H????: ?S?[??????n?_??H??A?n?_?Z????????????]????B
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

    TickStatus Action::ChasePlayerForPathFinding::DoTick(const TickContext& context)
    {
        const auto grid = heightGridMap_.get();
        if (!grid)
            return TickStatus::Failure;

        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();

        std::vector<glm::vec3> playerPositions;
        for (const auto& player : context.AllPlayer())
            playerPositions.push_back(player.lock()->PlayerTransform().GetWorldPos());

        if (playerPositions.empty())
            return TickStatus::Failure;

        // �����ς݂̒T�����ʂ���荞��
        if (isReady_.load(std::memory_order_acquire))
        {
            {
                std::lock_guard lock(mutex_);
                cachedPath_ = resultPath_;
            }
            hasPath_     = !cachedPath_.empty();
            isReady_     = false;
            isSearching_ = false; // ???t???[????V????T?????N?????????????
        }

        if (!isSearching_)
        {
            searchTimer_ -= Time::DeltaTime();
            if (searchTimer_ <= 0.0f)
            {
                searchTimer_ = (std::max)(0.0f, searchIntervalSec_);
                isSearching_ = true;
                if (pathThread_.joinable())
                    pathThread_.join();

                const auto  gridShared = grid;
                const int   range      = maxPathCellRange_;
                const float angle      = maxClimbAngleDeg_;
                pathThread_ = std::thread([this, gridShared, selfPos, playerPositions, range, angle]()
                {
                    std::vector<glm::vec3> bestPath;
                    for (const auto& goalPos : playerPositions)
                    {
                        auto path = FindPath(*gridShared, selfPos, goalPos, range, angle);
                        if (!path.empty() && (bestPath.empty() || path.size() < bestPath.size()))
                            bestPath = std::move(path);
                    }
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        resultPath_ = std::move(bestPath);
                    }
                    isReady_.store(true, std::memory_order_release);
                });
            }
        }

        /** ?O???T????????????????? */
        // ????T???????????????o?H??????????????? Failure ?????B
        if (!hasPath_ || cachedPath_.empty())
            return TickStatus::Failure;

        // ?O?t???[??????v?Z?????o?H???????p?_???????
        // ?????u?????????B??????p?_???????A???????????????????p?_??i???B
        // ????? searchIntervalSec_ ??????????g???u?X?V???]?????B
        const glm::vec2 cellSize        = grid->CellSize();
        const float     halfCell        = 0.5f * (std::min)(std::abs(cellSize.x), std::abs(cellSize.y));
        const float     arrivalRadius   = (std::max)(halfCell, moveSpeed_ * Time::DeltaTime() * 1.5f);
        const float     arrivalRadiusSq = arrivalRadius * arrivalRadius;

        while (!cachedPath_.empty())
        {
            glm::vec3 toWaypoint = cachedPath_.front() - selfPos;
            toWaypoint.y = 0.0f;
            if (toWaypoint.x * toWaypoint.x + toWaypoint.z * toWaypoint.z > arrivalRadiusSq)
                break;
            cachedPath_.erase(cachedPath_.begin()); // ???B????????????p?_??
        }

        // ?S???p?_????B??????o?H???g?????????????T??????
        if (cachedPath_.empty())
        {
            hasPath_ = false;
            return TickStatus::Failure;
        }

        const glm::vec3 target   = cachedPath_.front();
        glm::vec3       toTarget = target - selfPos;
        toTarget.y = 0.0f;

        const float horizDistSq = toTarget.x * toTarget.x + toTarget.z * toTarget.z;
        if (horizDistSq <= 1e-6f)
            return TickStatus::Failure;

        toTarget = glm::normalize(toTarget);
        glm::vec3 velocity = toTarget * moveSpeed_;
        velocity.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y; // ?d?????????
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), velocity);

        // ??????????]??????
        auto& transform = context.EnemyTransform();
        glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        forward.y = 0.0f;
        if (glm::length2(forward) > 0.0001f)
        {
            forward = glm::normalize(forward);

            const float dot      = glm::clamp(glm::dot(forward, toTarget), -1.0f, 1.0f);
            const float angleRad = std::acos(dot);
            if (angleRad > glm::radians(rotateToleranceDeg_))
            {
                const glm::quat currentRot = transform.GetWorldRot();
                const glm::quat deltaRot = dot < -0.9999f
                    ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
                    : glm::rotation(forward, toTarget);

                const float maxStep = glm::radians(rotateSpeed_) * Time::DeltaTime();
                const float t       = glm::min(1.0f, maxStep / angleRad);
                transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
            }
        }

        if (animationNumber_ >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);

        return TickStatus::Success;
    }

    void Action::ChasePlayerForPathFinding::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("heightGridMap_", heightGridMap_);
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_", rotateSpeed_);
        ImGuiHelper::OnDrawInputField("rotateToleranceDeg_", rotateToleranceDeg_);
        ImGuiHelper::OnDrawInputField("maxPathCellRange_", maxPathCellRange_);
        ImGuiHelper::OnDrawInputField("maxClimbAngleDeg_", maxClimbAngleDeg_);
        ImGuiHelper::OnDrawInputField("searchIntervalSec_", searchIntervalSec_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
    }
}
