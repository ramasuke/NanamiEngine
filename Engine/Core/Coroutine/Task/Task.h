#pragma once
#include <coroutine>

#include "../../Application/ApplicationBase.h"
#include "../../Application/Window/Main/Game/GameWindow.h"
#include "../Scheduler/CoroutineScheduler.h"

namespace Coroutine
{
    template <typename T>
    class Task final
    {
    public:
        struct promise_type
        {
            T value_;

            Task get_return_object()
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_value(T value) { value_ = value; }

            void unhandled_exception()
            {
            }

            T& result() { return value_; }
        };

        explicit Task(std::coroutine_handle<promise_type> h) : handle_(h)
        {
        }

        [[nodiscard]] bool await_ready() const noexcept { return handle_.done(); }

        void await_suspend(std::coroutine_handle<> awaiting) const;

        T await_resume() const noexcept
        {
            return handle_.promise().result();
        }

        void Resume() const
        {
            if (!handle_.done())
            {
                handle_.resume();
            }
        }

    private:
        std::coroutine_handle<promise_type> handle_;
    };

    template <typename T>
    void Task<T>::await_suspend(std::coroutine_handle<> awaiting) const
    {
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterTask(handle_, awaiting);
    }

    template <>
    class Task<void>
    {
    public:
        struct promise_type
        {
            Task get_return_object()
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_void()
            {
            }

            void unhandled_exception()
            {
            }

            void await_resume() const noexcept
            {
            }
        };

        explicit Task(const std::coroutine_handle<promise_type> h) : handle_(h)
        {
        }

        [[nodiscard]] bool await_ready() const noexcept { return handle_.done(); }

        void await_suspend(const std::coroutine_handle<> awaiting) const
        {
            Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterTask(handle_, awaiting);
        }

        void await_resume() const noexcept
        {
        }

        void Resume() const
        {
            if (!handle_.done()) handle_.resume();
        }

    private:
        std::coroutine_handle<promise_type> handle_;
    };
}
