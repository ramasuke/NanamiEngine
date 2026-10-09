#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <concepts>
#include <memory>

#include "../../../../Module/LifeCycleCallback/Awake/IAwakable.h"
#include "../../../../Module/LifeCycleCallback/BeginPhysics/IBeginPhysics.h"
#include "../../../../Module/LifeCycleCallback/FixedUpdate/IFixedUpdatable.h"
#include "../../../../Module/LifeCycleCallback/PreFixedUpdate/IPreFixedUpdate.h"
#include "../../../../Module/LifeCycleCallback/Group/LifeCycleCallbackGroup.h"
#include "../../../../Module/LifeCycleCallback/Group/OnceCallbackGroup/LifeCycleOnceCallbackGroup.h"
#include "../../../../Module/LifeCycleCallback/Group/SortCallbackGroup/LifeCycleSortCallbackGroup.h"
#include "../../../../Module/LifeCycleCallback/GuidRenderer/IDebugRenderable.h"
#include "../../../../Module/LifeCycleCallback/InitRenderable/IInitRenderable.h"
#include "../../../../Module/LifeCycleCallback/LateUpdate/LateUpdate.h"
#include "../../../../Module/LifeCycleCallback/Render/IRenderable.h"
#include "../../../../Module/LifeCycleCallback/ShadowRenderable/IShadowRenderable.h"
#include "../../../../Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../../Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../Module/LifeCycleCallback/UpdatedPhysics/IEndPhysics.h"
#include "../../../../Module/LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"
#include "../../../../Module/LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable_PriorityOrder.h"

namespace Coroutine
{
    class CoroutineScheduler;
}

namespace NanamiEngine::Core::Application
{
    // NOTE: Window のライフサイクル。Awake → Start → 物理 → Update → 描画の順に各コールバックを呼ぶ
    class NANAMI_API WindowLifeCycle final
    {
    public:
        explicit WindowLifeCycle(bool useShadowMap);
        ~WindowLifeCycle();

        void OnUpdateForGame();
        // NOTE: 更新や物理は回さず、描画だけ行う
        void OnUpdateForEditor();

        // NOTE: T の静的な型から対応するコールバック群へ登録する
        template<typename T>
        void StaticAddCallback (std::weak_ptr<T> add);
        // NOTE: dynamic_cast で実際の型を見て対応するコールバック群へ登録する
        template<typename T>
        void DynamicAddCallback(std::weak_ptr<T> add);
        std::unique_ptr<Coroutine::CoroutineScheduler>& Coroutine() { return coroutineScheduler_; }

        void InitRenderableAddedContentPop();
        void AwakableAddedContentPop();
        void StartableAddedContentPop();
        
    private:
        void UpdateShadowMapDrawArea() const;

        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IInitRenderable> initRenderableCallbacks_;
        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IAwakable>       awakableCallbacks_;
        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IStartable>      startableCallbacks_;

        std::unique_ptr<Coroutine::CoroutineScheduler>                                  coroutineScheduler_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IUpdatable>                   updatableCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::ILateUpdatable>               lateUpdatableCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IFixedUpdatable>              fixedUpdatableCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IPreFixedUpdate>              preFixedUpdateCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IBeginPhysics>                beginPhysicsCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IEndPhysics>                  endPhysicsCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IRenderable>                  renderableCallbacks_;
        LifeCycleSortCallbackGroup<Module::LifeCycleCallback::IUserInterfaceRenderable, Module::LifeCycleCallback::UiRenderableOrderCompare> uiRenderableCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IShadowRenderable>            shadowRenderableCallbacks_;
        LifeCycleCallbackGroup<Module::LifeCycleCallback::IDebugRenderable>             guiRenderableCallbacks_;
        
        int shadowMapDxLibHandle_ = -1;
        float accumulator_    = 0.0f;
        bool  hasDeferredPushedContents_ = false;
    };

    template <typename T>
    void WindowLifeCycle::StaticAddCallback(std::weak_ptr<T> add)
    {
        using CallbackType = T;
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IInitRenderable>)
            initRenderableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IAwakable>)
            awakableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IStartable>)
            startableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IUpdatable>)
            updatableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::ILateUpdatable>)
            lateUpdatableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IFixedUpdatable>)
            fixedUpdatableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IPreFixedUpdate>)
            preFixedUpdateCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IBeginPhysics>)
            beginPhysicsCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IEndPhysics>)
            endPhysicsCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IShadowRenderable>)
            shadowRenderableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IRenderable>)
            renderableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IUserInterfaceRenderable>)
            uiRenderableCallbacks_.Add(add);
        if constexpr (std::derived_from<CallbackType, Module::LifeCycleCallback::IDebugRenderable>)
            guiRenderableCallbacks_.Add(add);
    }

    template <typename T>
    void WindowLifeCycle::DynamicAddCallback(std::weak_ptr<T> add)
    {
        if (auto shared = add.lock())
        {
            if (auto initRenderable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IInitRenderable>(shared))
                initRenderableCallbacks_  .Add(initRenderable);
            if (auto awakable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IAwakable>(shared))
                awakableCallbacks_        .Add(awakable);
            if (auto startable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IStartable>(shared))
                startableCallbacks_       .Add(startable);
            if (auto updatable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IUpdatable>(shared))
                updatableCallbacks_       .Add(updatable);
            if (auto lateUpdatable = std::dynamic_pointer_cast<Module::LifeCycleCallback::ILateUpdatable>(shared))
                lateUpdatableCallbacks_   .Add(lateUpdatable);
            if (auto fixedUpdatable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IFixedUpdatable>(shared))
                fixedUpdatableCallbacks_  .Add(fixedUpdatable);
            if (auto preFixedUpdate = std::dynamic_pointer_cast<Module::LifeCycleCallback::IPreFixedUpdate>(shared))
                preFixedUpdateCallbacks_  .Add(preFixedUpdate);
            if (auto beginPhysics = std::dynamic_pointer_cast<Module::LifeCycleCallback::IBeginPhysics>(shared))
                beginPhysicsCallbacks_    .Add(beginPhysics);
            if (auto endPhysics = std::dynamic_pointer_cast<Module::LifeCycleCallback::IEndPhysics>(shared))
                endPhysicsCallbacks_      .Add(endPhysics);
            if (auto shadowRenderable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IShadowRenderable>(shared))
                shadowRenderableCallbacks_.Add(shadowRenderable);
            if (auto renderable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IRenderable>(shared))
                renderableCallbacks_      .Add(renderable);
            if (auto renderable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IUserInterfaceRenderable>(shared))
                uiRenderableCallbacks_    .Add(renderable);
            if (auto guiRenderable = std::dynamic_pointer_cast<Module::LifeCycleCallback::IDebugRenderable>(shared))
                guiRenderableCallbacks_   .Add(guiRenderable);
        }
    }
}
