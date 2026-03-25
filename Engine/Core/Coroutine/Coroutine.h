#pragma once
#include "FutureTask/FutureTask.h"
#include "Task/Task.h"

namespace Coroutine
{
    template <typename T>
    void StartCoroutine(Task<T> task)
    {
        task.Resume();
    }

    #define future_start(method) Coroutine::StartFuture([this] { return method(); })
    
    template <typename Func, typename T = std::invoke_result_t<Func>>
    FutureTask<T> StartFuture(Func&& taskFactory)
    {
        auto future = std::make_shared<std::future<T>>(
            std::async(std::launch::async, [factory = std::forward<Func>(taskFactory)] {
                auto task = factory();
                task.Resume();
                return task.Result();
            })
        );
        return FutureTask<T>{future};
    }
};
