#include "WaitForSeconds.h"

#include "../../../Application/ApplicationBase.h"
#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"

Coroutine::WaitForSeconds::WaitForSeconds(const float duration_secs): awaitTimeDuration_secs_(duration_secs)
{
    
}

bool Coroutine::WaitForSeconds::await_ready() const noexcept
{
    return awaitTimeDuring_secs_ >= awaitTimeDuration_secs_;
}

void Coroutine::WaitForSeconds::await_suspend(const std::coroutine_handle<> parentHandle)
{
    parentHandle_ = parentHandle;
    Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterWaitForSeconds(this);
}

void Coroutine::WaitForSeconds::Tick(const float deltaTime_secs)
{
    awaitTimeDuring_secs_ += deltaTime_secs;
}
