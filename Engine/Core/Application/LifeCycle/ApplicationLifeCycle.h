#pragma once
#include "../../../Module/LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../../../Module/LifeCycleCallback/Group/OnceCallbackGroup/LifeCycleOnceCallbackGroup.h"
#include "../../Object/Field/Interface/IFieldContext.h"

namespace NanamiEngine::Core::Application
{
    class ApplicationLifeCycle final
    {
    public:
        void OnUpdate();
        void OnUpdateFieldInittables();

        template<typename T>
        void AddCallback(std::weak_ptr<T> add);
        
    private:
        LifeCycleOnceCallbackGroup<Object::IFieldContext> fieldInitableCallbacks_;
        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IEnablableAsset> enableAssetCallbacks_;
    };

    template <typename T>
    void ApplicationLifeCycle::AddCallback(std::weak_ptr<T> add)
    {
        if constexpr (std::derived_from<T, Module::LifeCycleCallback::IEnablableAsset>)
        {
            enableAssetCallbacks_.Add(add);
        }
        if constexpr (std::derived_from<T, Object::IFieldContext>)
        {
            fieldInitableCallbacks_.Add(add);
        }
    }
}
