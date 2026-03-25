#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>

#include "../SharedHash/SharedHash.h"

namespace NanamiEngine::Core::Application
{
    template <typename T>
    class LifeCycleSortCallbackGroup final 
    {
    public:
        explicit LifeCycleSortCallbackGroup(std::function<bool(const std::weak_ptr<T>&,
                                            const std::weak_ptr<T>&)> onSortFunction);
        void Add   (std::weak_ptr<T> add);
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::queue<std::weak_ptr<T>> addContentQueue_   ;
        std::queue<std::weak_ptr<T>> removeContentQueue_;
        std::unordered_set<std::weak_ptr<T>, SharedPtrHash<T>, SharedPtrEqual<T>> contents_;
        const std::function<bool(const std::weak_ptr<T>&, const std::weak_ptr<T>&)> onSortFunction_;
    };


    template <typename T>
    LifeCycleSortCallbackGroup<T>::LifeCycleSortCallbackGroup(
    std::function<bool(const std::weak_ptr<T>&,
                       const std::weak_ptr<T>&)> onSortFunction)
    : onSortFunction_(onSortFunction)
    {
    }


    template <typename T>
    void LifeCycleSortCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }

    template <typename T>
    void LifeCycleSortCallbackGroup<T>::OnUpdatePushedContents()
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
    void LifeCycleSortCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        std::vector<std::weak_ptr<T>> contents(contents_.begin(), contents_.end());
        std::sort(contents.begin(), contents.end(),
                  [this](const std::weak_ptr<T>& a, const std::weak_ptr<T>& b)
        {
            return onSortFunction_(a, b);
        });

        for (const auto& content : contents)
        {
            if (auto sp = content.lock())
            {
                func(*sp);
            }
            else
            {
                removeContentQueue_.push(content);
            }
        }
    }
}