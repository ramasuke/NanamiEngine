#pragma once
#include <functional>
#include <memory>
#include <stack>

namespace NanamiEngine::Core::Application
{
    template <typename T>
    class LifeCycleOnceCallbackGroup final
    {
    public:
        void Add(std::weak_ptr<T> add);
        void AddedContentPop();
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::stack<std::weak_ptr<T>> addContentStack_;
        std::stack<std::weak_ptr<T>> contents_;
    };

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentStack_.push(add);
    }

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::AddedContentPop()
    {
        if (!addContentStack_.empty())
        {
            addContentStack_.pop();
        }
    }

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::OnUpdatePushedContents()
    {
        while (!addContentStack_.empty())
        {
            auto wp = addContentStack_.top();
            contents_.push(wp);
            addContentStack_.pop();
        }
    }

    template <typename T>
    void LifeCycleOnceCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        while (!contents_.empty())
        {
            auto shared = contents_.top().lock();
            contents_.pop();

            if (shared)
            {
                func(*shared);
            }
        }
    }
}