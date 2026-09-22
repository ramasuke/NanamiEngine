#pragma once
#include <coroutine>
#include <functional>
#include <type_traits>

#include "../Engine_Coroutine_ITickableWaitable.h"
#include "../TweenClock/Coroutine_TweenClock.h"
#include "../tweeny/Tweeny/tween.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"

namespace Coroutine
{
    template <typename T>
    class WaitForTweenV final : public ITickableWaitable
    {
    public:
        explicit WaitForTweenV(T& target, tweeny::tween<T> tween)
            : apply_([&target](const T& value) { target = value; })
            , tween_(std::move(tween))
        {}

        // setter 経由でしか書けない値用(例: TextRenderer::SetTextColor)。T は tween から推論する
        WaitForTweenV(std::type_identity_t<std::function<void(const T&)>> apply, tweeny::tween<T> tween)
            : apply_(std::move(apply))
            , tween_(std::move(tween))
        {}

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return tween_.isFinished();
        }

        void await_suspend(std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;

            Core::Application::ApplicationBase::GameWindow()
                ->LifeCycle().Coroutine()
                ->RegisterTickable(this);
        }

        void await_resume() const noexcept {}

        void Tick(float deltaTime) override
        {
            tween_.step(clock_.Advance(deltaTime));

            apply_(tween_.peek());
        }

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        std::function<void(const T&)> apply_;
        tweeny::tween<T> tween_;
        TweenClock clock_;
        std::coroutine_handle<> parentHandle_{};
    };
}
