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

    std::vector<glm::vec3> Action::ChasePlayerForPathFinding::FindPath(
        const NanamiEngine::Module::Asset::HeightGridMap& grid,
        const glm::vec3& start, const glm::vec3& goal,
        const int maxCellRange, const float maxClimbAngleDeg)
    {
        int sx, sz, gx, gz;
        if (!grid.WorldToCell(start, sx, sz)) return {}; 
        if (!grid.WorldToCell(goal,  gx, gz)) return {};
        if (sx == gx && sz == gz) return {};

        const int W = grid.DivisionsX();
        const int H = grid.DivisionsZ();

        const glm::vec2 cell      = grid.CellSize();
        const float     orthoDist = (std::min)(std::abs(cell.x), std::abs(cell.y));
        // 登坂可能傾斜の正接。 高低差 <= maxSlopeTan * 水平距離 なら通行可能
        const float     maxSlopeTan = std::tan(maxClimbAngleDeg * PI / 180.0f);

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

        static constexpr int DX[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
        static constexpr int DZ[8] = {  0,  0,  1, -1,  1, -1,  1, -1 };

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
                const int nx = current.x + DX[d];
                const int nz = current.z + DZ[d];

                if (!inBounds(nx, nz))
                    continue;
                // 探索を許可するセル範囲を超えたら除外
                if (std::abs(nx - sx) > maxCellRange || std::abs(nz - sz) > maxCellRange)
                    continue;

                const bool  diagonal = (DX[d] != 0 && DZ[d] != 0);
                const float horiz    = diagonal ? orthoDist * SQRT2 : orthoDist;
                if (horiz <= 0.0f)
                    continue;

                // 傾斜判定
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

        // 経路復元: ゴールから始点へ辿り、始点セルを除いて反転する。
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

        const glm::vec3 selfPos   = context.EnemyTransform().GetWorldPos();
        const glm::vec3 playerPos = context.Player()->PlayerTransform().GetWorldPos();

        // 完了済みの探索結果を取り込む
        if (isReady_.load(std::memory_order_acquire))
        {
            {
                std::lock_guard lock(mutex_);
                cachedPath_ = resultPath_;
            }
            hasPath_     = !cachedPath_.empty();
            isReady_     = false;
            isSearching_ = false; // 次フレームで新たな探索を起動できるようにする
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
                pathThread_ = std::thread([this, gridShared, selfPos, playerPos, range, angle]()
                {
                    auto path = FindPath(*gridShared, selfPos, playerPos, range, angle);
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        resultPath_ = std::move(path);
                    }
                    isReady_.store(true, std::memory_order_release);
                });
            }
        }

        /** 前回の探索に成功していれば移動 */
        // 初回探索が完了するまでは経路が無いので移動せず Failure を返す。
        if (!hasPath_ || cachedPath_.empty())
            return TickStatus::Failure;

        // 前フレームまでに計算した経路の次の中継点へ向かう
        // 現在位置に応じて到達済みの中継点を消費し、移動した分だけ次の中継点へ進める。
        // これで searchIntervalSec_ を待たずに自身の位置更新へ追従できる。
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
            cachedPath_.erase(cachedPath_.begin()); // 到達したので次の中継点へ
        }

        // 全中継点に到達したら経路を使い切ったので次の探索を待つ
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
        velocity.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y; // 重力成分は維持
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), velocity);

        // 移動方向へ回転させる
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
                const glm::quat deltaRot = (dot < -0.9999f)
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
