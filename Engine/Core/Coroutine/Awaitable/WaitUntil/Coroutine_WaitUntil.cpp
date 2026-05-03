#include "Coroutine_WaitUntil.h"

#include "../../Scheduler/CoroutineScheduler.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"

namespace Coroutine
{
    WaitUntil::WaitUntil(std::function<bool()> condition)
        : condition_(std::move(condition))
    {
    }

    bool WaitUntil::await_ready() const noexcept
    {
        return condition_();
    }

    void WaitUntil::await_suspend(const std::coroutine_handle<> parentHandle)
    {
        parentHandle_ = parentHandle;

        Core::Application::ApplicationBase::GameWindow()
            ->LifeCycle().Coroutine()
            ->RegisterTickable(this);
    }

    void WaitUntil::Tick(float)
    {
    }
}
