#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <typeinfo>

#include "SharedHash/SharedHash.h"
#include "LifeCycle_CallbackableType.h"
#include "../../SafeExecute/Engine_Module_SafeExecute.h"
#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Application
{
    template <LifeCycleCallbackType T>
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

    template <LifeCycleCallbackType T>
    void LifeCycleCallbackGroup<T>::Add(std::weak_ptr<T> add)
    {
        addContentQueue_.push(add);
    }

    template <LifeCycleCallbackType T>
    void LifeCycleCallbackGroup<T>::Invoke(const std::function<void(T&)>& func)
    {
        for (const auto& weakPtr : contents_)
        {
            if (auto sharedPtr = weakPtr.lock())
            {
                T* rawPtr = sharedPtr.get();
                std::string errorMessage;
                if (!Module::SafeExecute([&func, rawPtr]() { func(*rawPtr); }, errorMessage))
                {
                    const std::string_view fullName = typeid(*rawPtr).name();
                    const size_t lastColon = fullName.rfind("::");
                    const std::string_view shortName = lastColon != std::string_view::npos ? fullName.substr(lastColon + 2) : fullName;
                    Module::LogError("[LifeCycleCallback] " + std::string(shortName) + " (" + rawPtr->GetGuid().Value() + "): " + errorMessage);
                }
            }
            else
            {
                removeContentQueue_.push(weakPtr);
            }
        }
    }

    template <LifeCycleCallbackType T>
    void LifeCycleCallbackGroup<T>::OnUpdatePushedContents()
    {
        while (!removeContentQueue_.empty())
        {
            auto& weakPtr = removeContentQueue_.front();
            contents_.erase(weakPtr);
            removeContentQueue_.pop();
        }
        
        while (!addContentQueue_.empty())
        {
            auto& weakPtr = addContentQueue_.front();
            contents_.insert(weakPtr);
            addContentQueue_.pop();
        }
    }
}
