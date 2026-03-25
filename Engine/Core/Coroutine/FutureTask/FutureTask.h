#pragma once
#include <coroutine>
#include <future>
#include <memory>
#include <thread>

#include "../../Application/ApplicationBase.h"
#include "../../Application/Window/Main/Game/GameWindow.h"

namespace Coroutine
{
    template <typename T>
    struct FutureTask
    {
        std::shared_ptr<std::future<T>> future_;
        std::coroutine_handle<> awaiting_;

        explicit FutureTask(std::shared_ptr<std::future<T>> future)
            : future_(std::move(future)) {}

        [[nodiscard]] bool await_ready() const noexcept;

        void await_suspend(std::coroutine_handle<> awaiting)
        {
            awaiting_ = awaiting;
            std::thread([future = future_, awaiting]() {
                future->wait();
                NanamiEngine::Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterFuture(awaiting);
            }).detach();
        }

        T await_resume()
        {
            return future_->get();
        }
    };

    template <typename T>
    bool FutureTask<T>::await_ready() const noexcept
    {
        return future_->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
}
