#include "Coroutine_WaitForSubscription.h"

#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"

namespace Coroutine
{
    WaitForSubscription::WaitForSubscription(rxcpp::composite_subscription subscription)
        : subscription_(std::move(subscription))
    {
        subscription_.add([this]
        {
            isReady_ = true;
        });
    }

    bool WaitForSubscription::await_ready() const noexcept
    {
        return isReady_;
    }

    void WaitForSubscription::await_suspend(const std::coroutine_handle<> parentHandle)
    {
        parentHandle_ = parentHandle;
        
        Core::Application::ApplicationBase::GameWindow()
            ->LifeCycle().Coroutine()
            ->RegisterEvent(this);
    }
}