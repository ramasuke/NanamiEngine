#pragma once
#include <coroutine>
#include <type_traits>

#include "../tweeny/Tweeny/tween.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"
#include "../WaitForTween/IWaitForTween.h"

namespace Coroutine
{
    template <typename T>
    class WaitForTweenV final : public IWaitForTween
    {
    public:
        explicit WaitForTweenV(T& target, tweeny::tween<T> tween)
            : target_(target), tween_(std::move(tween))
        {}

        // ---- coroutine ----
        bool await_ready() const noexcept override
        {
            if (tween_.isFinished())
            {
                ApplyTween(target_, tween_.peek());
                return true;
            }
            return false;
        }

        void await_suspend(const std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;
            Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterWaitForTween(this);
        }

        void await_resume() const noexcept
        {
            
        }

        void Tick(int deltaTime_msecs) override
        {
            tween_.step(deltaTime_msecs);
            ApplyTween(target_, tween_.peek());

            if (tween_.isFinished())
            {
                parentHandle_.resume();
            }
        }

        std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        T& target_;

        tweeny::tween<T> tween_;
        std::coroutine_handle<> parentHandle_;

        static void ApplyTween(T& dst, const T& value)
        {
            dst = value;
        }
    };
}
