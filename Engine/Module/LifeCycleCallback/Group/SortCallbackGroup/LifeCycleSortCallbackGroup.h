#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>
#include "../LifeCycle_CallbackableType.h"

namespace NanamiEngine::Core::Application
{
    template <LifeCycleCallbackType T>
    class LifeCycleSortCallbackGroup final 
    {
    public:
        explicit LifeCycleSortCallbackGroup(
            std::function<bool(const std::shared_ptr<T>&,
                               const std::shared_ptr<T>&)> onSortFunction);

        void Add(std::weak_ptr<T> add);
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::queue<std::weak_ptr<T>> addContentQueue_;
        std::queue<Guid> removeContentQueue_;
        std::unordered_map<Guid, std::weak_ptr<T>, GuidHash> contents_;

        const std::function<bool(const std::shared_ptr<T>&,
                                 const std::shared_ptr<T>&)> onSortFunction_;
    };


    template <LifeCycleCallbackType T>
    LifeCycleSortCallbackGroup<T>::LifeCycleSortCallbackGroup(
        std::function<bool(const std::shared_ptr<T>&,
                           const std::shared_ptr<T>&)> onSortFunction)
        : onSortFunction_(onSortFunction)
    {
    }

    template <LifeCycleCallbackType T>
    void LifeCycleSortCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }


    template <LifeCycleCallbackType T>
    void LifeCycleSortCallbackGroup<T>::OnUpdatePushedContents()
    {
        // 追加
        while (!addContentQueue_.empty())
        {
            auto& wp = addContentQueue_.front();

            if (auto sp = wp.lock())
            {
                contents_[sp->GetGuid()] = wp;
            }

            addContentQueue_.pop();
        }

        // 削除
        while (!removeContentQueue_.empty())
        {
            contents_.erase(removeContentQueue_.front());
            removeContentQueue_.pop();
        }
    }


    template <LifeCycleCallbackType T>
    void LifeCycleSortCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        std::vector<std::shared_ptr<T>> sorted;
        sorted.reserve(contents_.size());

        // lockして有効なものだけ収集
        for (auto& [guid, wp] : contents_)
        {
            if (auto sp = wp.lock())
            {
                sorted.push_back(sp);
            }
            else
            {
                removeContentQueue_.push(guid);
            }
        }

        // ソート
        std::sort(sorted.begin(), sorted.end(), onSortFunction_);

        // 実行
        for (auto& sharedPtr : sorted)
        {
            func(*sharedPtr);
        }
    }
}