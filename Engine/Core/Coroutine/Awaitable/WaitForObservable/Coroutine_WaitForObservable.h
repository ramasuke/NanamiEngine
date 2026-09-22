#pragma once
#include <coroutine>

#include "Coroutine_IWaitForObservable.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"
#include "../../../../../Packages/R4/R4.h"

namespace Coroutine
{
    template<typename ArgT>
    struct WaitForObservable final : IWaitForObservable 
    {
    public:
        explicit WaitForObservable(NanamiEngine::R4::Observable<ArgT> observable)
            : observable_(observable)
        {
            subscription_ = observable_.Subscribe(
                [this](const ArgT&)
                {
                    isReady_ = true;
                });
        }

        ~WaitForObservable() override
        {
            subscription_.Dispose();
        }

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return isReady_;
        }

        void await_suspend(std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;

            Core::Application::ApplicationBase::GameWindow()
                ->LifeCycle().Coroutine()
                ->RegisterEvent(this);
        }

        void await_resume() const noexcept {}

        std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        NanamiEngine::R4::Disposable subscription_;
        std::coroutine_handle<> parentHandle_;
        NanamiEngine::R4::Observable<ArgT> observable_;
        bool isReady_ = false;
    };
}