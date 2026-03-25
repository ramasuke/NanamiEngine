#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>

#include "SharedHash/SharedHash.h"

namespace NanamiEngine::Core::Application
{
    template <typename T>
    class LifeCycleCallbackGroup final
    {
    public:
        void Add (std::weak_ptr<T> add);
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::queue<std::weak_ptr<T>> addContentQueue_   ;
        std::queue<std::weak_ptr<T>> removeContentQueue_;
        std::unordered_set<std::weak_ptr<T>, SharedPtrHash<T>, SharedPtrEqual<T>> contents_;
    };

    template <typename T>
    void LifeCycleCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }

    template <typename T>
    void LifeCycleCallbackGroup<T>::OnUpdatePushedContents()
    {
        while (!addContentQueue_.empty())
        {
            auto& wp = addContentQueue_.front();
            contents_.insert(wp);
            addContentQueue_.pop();
        }

        while (!removeContentQueue_.empty())
        {
            auto& wp = removeContentQueue_.front();
            contents_.erase(wp);
            removeContentQueue_.pop();
        }
    }

    template <typename T>
    void LifeCycleCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        for (const auto& wp : contents_)
        {
            if (auto sp = wp.lock())
            {
                func(*sp);
            }
            else
            {
                removeContentQueue_.push(wp);
            }
        }
    }
}
