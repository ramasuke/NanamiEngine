#include "Coroutine_WaitYield.h"

#include "../../Scheduler/CoroutineScheduler.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"

namespace Coroutine
{
    void WaitYield::await_suspend(const std::coroutine_handle<> parentHandle)
    {
        parentHandle_ = parentHandle;

        Core::Application::ApplicationBase::GameWindow()
            ->LifeCycle().Coroutine()
            ->RegisterTickable(this);
    }

    void WaitYield::Tick(float)
    {
        if (firstTick_)
        {
            firstTick_ = false;
            return;
        }

        completed_ = true;
    }
}