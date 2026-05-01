#pragma once
#include <coroutine>

#include "../Engine_Coroutine_ITickableWaitable.h"
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
            : target_(target)
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
            const int dt_ms = static_cast<int>(deltaTime * 1000.0f);

            tween_.step(dt_ms);

            ApplyTween(target_, tween_.peek());
        }

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        T& target_;
        tweeny::tween<T> tween_;
        std::coroutine_handle<> parentHandle_{};

        static void ApplyTween(T& dst, const T& value)
        {
            dst = value;
        }
    };
}