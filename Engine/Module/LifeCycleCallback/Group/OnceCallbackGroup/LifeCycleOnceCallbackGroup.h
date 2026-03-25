#pragma once
#include <functional>
#include <memory>
#include <queue>

namespace NanamiEngine::Core::Application
{
    template <typename T>
    class LifeCycleOnceCallbackGroup final
    {
    public:
        void Add(std::weak_ptr<T> add);
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::queue<std::weak_ptr<T>> addContentQueue_;
        std::queue<std::weak_ptr<T>> contents_;
    };

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::OnUpdatePushedContents()
    {
        while (!addContentQueue_.empty())
        {
            auto& wp = addContentQueue_.front();
            contents_.push(wp);
            addContentQueue_.pop();
        }
    }

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        while (!contents_.empty())
        {
            auto shared = contents_.front().lock();
            contents_.pop();

            if (shared)
            {
                func(*shared);
            }
            // else: 破棄済みなので何もしない
        }
    }
}
