#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include "../LifeCycle_CallbackableType.h"
#include "../../../SafeExecute/Engine_Module_SafeExecute.h"
#include "../../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Application
{
    template <LifeCycleCallbackType T,
              typename Compare = std::function<bool(const std::shared_ptr<T>&,
                                                    const std::shared_ptr<T>&)>>
    class LifeCycleSortCallbackGroup final
    {
    public:
        explicit LifeCycleSortCallbackGroup(Compare compare = Compare{});

        void Add(std::weak_ptr<T> add);
        void Invoke(const std::function<void(T&)>& func);
        void OnUpdatePushedContents();

    private:
        std::queue<std::weak_ptr<T>> addContentQueue_;
        std::queue<Guid> removeContentQueue_;
        std::unordered_map<Guid, std::weak_ptr<T>, GuidHash> contents_;

        Compare comparator_;
    };


    template <LifeCycleCallbackType T, typename Compare>
    LifeCycleSortCallbackGroup<T, Compare>::LifeCycleSortCallbackGroup(Compare compare)
        : comparator_(std::move(compare))
    {
    }

    template <LifeCycleCallbackType T, typename Compare>
    void LifeCycleSortCallbackGroup<T, Compare>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }


    template <LifeCycleCallbackType T, typename Compare>
    void LifeCycleSortCallbackGroup<T, Compare>::OnUpdatePushedContents()
    {
        // 削除
        while (!removeContentQueue_.empty())
        {
            contents_.erase(removeContentQueue_.front());
            removeContentQueue_.pop();
        }
        
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
    }

    template <LifeCycleCallbackType T, typename Compare>
    void LifeCycleSortCallbackGroup<T, Compare>::Invoke(const std::function<void(T&)>& func)
    {
        std::vector<std::shared_ptr<T>> sorted;
        sorted.reserve(contents_.size());

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

        std::sort(sorted.begin(), sorted.end(), comparator_);

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            std::weak_ptr<T> wp = sorted[i];
            sorted[i].reset();
            if (auto sp = wp.lock())
            {
                T* rawPtr = sp.get();
                std::string errorMessage;
                if (!Module::SafeExecute([&func, rawPtr]() { func(*rawPtr); }, errorMessage))
                {
                    const std::string_view fullName = typeid(*rawPtr).name();
                    const size_t lastColon = fullName.rfind("::");
                    const std::string_view shortName = lastColon != std::string_view::npos ? fullName.substr(lastColon + 2) : fullName;
                    Module::LogError("[LifeCycleCallback] " + std::string(shortName) + " (" + rawPtr->GetGuid().Value() + "): " + errorMessage);
                }
            }
        }
    }
}