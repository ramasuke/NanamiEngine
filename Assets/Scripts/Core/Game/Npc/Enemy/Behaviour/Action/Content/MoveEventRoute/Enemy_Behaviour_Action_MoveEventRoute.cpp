#include "Enemy_Behaviour_Action_MoveEventRoute.h"

#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/WaitForTween.h"
#include "../../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../../../Libs/LibCore/Tween/Ease/Type/EaseType.h"
#include "../../../../../../../../../Data/EventNpcWalkingRoute/RoutePoint/Data_Event_Npc_WalkingRoute_RoutePoint.h"

namespace
{
    void RotateTowardsDirY(
        GameObject::Transform& transform,
        const glm::vec3& direction,
        const float rotateSpeedDeg)
    {
        glm::vec3 targetForward(direction.x, 0.0f, direction.z);
        if (glm::length2(targetForward) < 0.0001f)
            return;
        targetForward = glm::normalize(targetForward);

        glm::vec3 currentForward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        currentForward.y = 0.0f;

        if (glm::length2(currentForward) < 0.0001f)
            return;
        currentForward = glm::normalize(currentForward);

        const float dot = glm::dot(currentForward, targetForward);

        const glm::quat currentRot = transform.GetWorldRot();

        if (dot > 0.9999f)
            return;

        if (dot < -0.9999f)
        {
            const glm::quat deltaRot = glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0));

            const glm::quat targetRot = deltaRot * currentRot;
            const float t = glm::min(1.0f, glm::radians(rotateSpeedDeg) * Time::DeltaTime() / glm::pi<float>());
            transform.SetWorldRot(glm::slerp(currentRot, targetRot, t));
            return;
        }

        const glm::quat deltaRot = glm::rotation(currentForward, targetForward);
        const float angleDiff = glm::angle(deltaRot);

        const float maxStep = glm::radians(rotateSpeedDeg) * Time::DeltaTime();
        const float t = glm::min(1.0f, maxStep / angleDiff);

        transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
    }
}

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::MoveEventRoute::DoTick(const TickContext& context)
    {
        if (isFinished_)
        {
            isRunning_ = false;

            if (!isOnceExecute_)
                isFinished_ = false;

            return TickStatus::Success;
        }

        if (!isRunning_)
        {
            if (isExecuted_ && isOnceExecute_)
                return TickStatus::Success;

            isExecuted_ = true;
            isRunning_  = true;
            currentRouteIndex_ = 0;

            Coroutine::StartCoroutine(MoveAsync(context));
        }

        // 進行方向へ徐々に回転
        if (isRotateToMoveDir_ && moveRoute_)
        {
            const auto& routes = moveRoute_->Get();

            if (currentRouteIndex_ < routes.size())
            {
                const glm::vec3 pos = context.EnemyTransform().GetWorldPos();
                const glm::vec3 targetPos = routes[currentRouteIndex_].Position();
                const glm::vec3 moveDir = targetPos - pos;

                RotateTowardsDirY(
                    context.EnemyTransform(),
                    moveDir,
                    rotateSpeedDeg_);
            }
        }

        return TickStatus::Running;
    }

    Coroutine::Task<void> Action::MoveEventRoute::MoveAsync(const TickContext context)
    {
        const auto& routes = moveRoute_->Get();

        for (currentRouteIndex_ = 0;
             currentRouteIndex_ < routes.size();
             ++currentRouteIndex_)
        {
            co_await TweenAsync(routes[currentRouteIndex_], context);
        }

        isFinished_ = true;
    }

    Coroutine::Task<void> Action::MoveEventRoute::TweenAsync(
        const Data::EventNpcWalkingRoute::RoutePoint& throughRoute,
        const TickContext context)
    {
        const auto tween = tweeny::from(context.EnemyTransform().GetWorldPos())
            .to(throughRoute.Position())
            .during(throughRoute.Duration_msecs())
            .via(Tween::Ease(EaseType::Linear));

        co_await Coroutine::WaitForTween(context.EnemyTransform(), tween);
    }

    void Action::MoveEventRoute::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("moveRoute_", moveRoute_);
        ImGuiHelper::OnDrawInputField("isOnceExecute_", isOnceExecute_);
        ImGuiHelper::OnDrawInputField("isRotateToMoveDir_", isRotateToMoveDir_);
        ImGuiHelper::OnDrawInputField("rotateSpeedDeg_", rotateSpeedDeg_);
    }
}
