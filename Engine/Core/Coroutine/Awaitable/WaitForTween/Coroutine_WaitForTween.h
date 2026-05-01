#pragma once
#include <coroutine>
#include <tuple>

#include "../Engine_Coroutine_ITickableWaitable.h"
#include "../../../../Module/GameObject/Transform/Transform.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"
#include "../glm/gtx/quaternion.hpp"
#include "../tweeny/Tweeny/tween.h"

namespace Coroutine
{
    template<typename... Types>
    class WaitForTween final : public ITickableWaitable
    {
    public:
        explicit WaitForTween(GameObject::Transform& transformRef,
                              tweeny::tween<Types...> tween)
            : tween_(std::move(tween))
            , transformRef_(transformRef)
        {}

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return tween_.progress() >= 1.0f;
        }

        void await_suspend(const std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;

            Core::Application::ApplicationBase::GameWindow()
                ->LifeCycle().Coroutine()
                ->RegisterTickable(this);
        }

        void await_resume() const noexcept {}

        void Tick(const float deltaTime) override
        {
            const int deltaTime_msecs = static_cast<int>(deltaTime * 1000.0f);

            tween_.step(deltaTime_msecs);

            ApplyTween(transformRef_, tween_.peek());
        }

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        tweeny::tween<Types...> tween_;
        std::coroutine_handle<> parentHandle_{};
        GameObject::Transform& transformRef_;

        static void ApplyTween(GameObject::Transform& transform, const glm::vec3& pos)
        {
            transform.SetWorldPos(pos);
        }

        static void ApplyTween(GameObject::Transform& transform, const glm::quat& rot)
        {
            transform.SetWorldRot(rot);
        }

        static void ApplyTween(GameObject::Transform& transform,
                               const std::tuple<glm::vec3, glm::quat>& values)
        {
            transform.SetWorldPos(std::get<0>(values));
            transform.SetWorldRot(std::get<1>(values));
        }
    };
}