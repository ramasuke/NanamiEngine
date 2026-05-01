#include "Coroutine_WaitForSeconds.h"

#include "../../../Application/ApplicationBase.h"
#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"

namespace Coroutine
{
    WaitForSeconds::WaitForSeconds(const float duration_secs)
        : awaitTimeDuration_secs_(duration_secs)
    {
    }

    bool WaitForSeconds::await_ready() const noexcept
    {
        return awaitTimeDuring_secs_ >= awaitTimeDuration_secs_;
    }

    void WaitForSeconds::await_suspend(const std::coroutine_handle<> parentHandle)
    {
        parentHandle_ = parentHandle;

        Core::Application::ApplicationBase::GameWindow()
            ->LifeCycle().Coroutine()
            ->RegisterTickable(this);
    }

    void WaitForSeconds::Tick(const float deltaTime_secs)
    {
        awaitTimeDuring_secs_ += deltaTime_secs;
    }
}