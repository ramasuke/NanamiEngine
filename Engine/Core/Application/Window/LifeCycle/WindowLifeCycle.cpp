#include "WindowLifeCycle.h"

#include "DxLib.h"
#include "../../ApplicationBase.h"
#include "../../../../Module/Asset/Asset.h"
#include "../../../Coroutine/Scheduler/CoroutineScheduler.h"
#include "../../../Physics/Physics.h"
#include "../../Time/Time.h"

namespace NanamiEngine::Core::Application
{
    WindowLifeCycle::WindowLifeCycle()
        : coroutineScheduler_(std::make_unique<Coroutine::CoroutineScheduler>())
        , uiRenderableCallbacks_{
            [](const std::weak_ptr<Module::LifeCycleCallback::IUserInterfaceRenderable>& weakA,
               const std::weak_ptr<Module::LifeCycleCallback::IUserInterfaceRenderable>& weakB)
            {
                const auto a = weakA.lock();
                const auto b = weakB.lock();
                if (!a || !b)
                    return false;
        
                return a->GetRenderOrder() < b->GetRenderOrder();
            }
        }
    {
        const VECTOR lightDir = VGet(-0.5f, -1.0f, -0.5f);
        SetLightDirection(lightDir);
    
        constexpr COLOR_F difColor = {1.0f, 1.0f, 1.0f, 1.0f};
        SetLightDifColor(difColor);
        shadowMapDxLibHandle_ = MakeShadowMap(1024, 1024);
        SetShadowMapLightDirection(shadowMapDxLibHandle_, VGet(-0.5f, -1.0f, -0.5f));
        SetShadowMapDrawArea(shadowMapDxLibHandle_, VGet(-100.0f, -100.0f, -100.0f), VGet(100.0f, 100.0f, 100.0f));
    }
    
    WindowLifeCycle::~WindowLifeCycle() = default;
    
    void WindowLifeCycle::OnUpdateForGame()
    {
        if (Module::Asset::Asset::IsLoadingResource())
            return;
        
        initRenderableCallbacks_  .Invoke([](auto& obj) { obj.InitRenderer();     });
        awakableCallbacks_        .Invoke([](auto& obj) { obj.OnAwake();          });
        startableCallbacks_       .Invoke([](auto& obj) { obj.OnStart();          });
        
        beginPhysicsCallbacks_    .Invoke([](auto& obj) { obj.OnBeginPhysics();   });
        ApplicationBase::Physics().Update(Time::DeltaTime());
        endPhysicsCallbacks_      .Invoke([](auto& obj) { obj.OnUpdatedPhysics(); });
    
        updatableCallbacks_       .Invoke([](auto& obj) { obj.OnUpdate();         });
        lateUpdatableCallbacks_   .Invoke([](auto& obj) { obj.OnLateUpdate();     });
        coroutineScheduler_      ->Invoke();
        
        ShadowMap_DrawSetup(shadowMapDxLibHandle_);
        shadowRenderableCallbacks_.Invoke([](auto& obj) { obj.OnShadowRender();   });
        ShadowMap_DrawEnd();
        SetUseShadowMap( 0, shadowMapDxLibHandle_) ;
        renderableCallbacks_      .Invoke([](auto& obj) { obj.OnRender();         });
        SetUseShadowMap( 0, -1);
    
        uiRenderableCallbacks_    .Invoke([](auto& obj) { obj.OnUserInterfaceRender(); });
        guiRenderableCallbacks_   .Invoke([](auto& obj) { obj.OnDebugRender        (); });
    
        initRenderableCallbacks_  .OnUpdatePushedContents();
        awakableCallbacks_        .OnUpdatePushedContents();
        startableCallbacks_       .OnUpdatePushedContents();
        beginPhysicsCallbacks_    .OnUpdatePushedContents();
        endPhysicsCallbacks_      .OnUpdatePushedContents();
        updatableCallbacks_       .OnUpdatePushedContents();
        lateUpdatableCallbacks_   .OnUpdatePushedContents();
        shadowRenderableCallbacks_.OnUpdatePushedContents();
        renderableCallbacks_      .OnUpdatePushedContents();
        uiRenderableCallbacks_    .OnUpdatePushedContents();
        guiRenderableCallbacks_   .OnUpdatePushedContents();
    }
    
    void WindowLifeCycle::OnUpdateForEditor()
    {
        initRenderableCallbacks_  .Invoke([](auto& obj) { obj.InitRenderer();   });
        
        ShadowMap_DrawSetup(shadowMapDxLibHandle_) ;
        shadowRenderableCallbacks_.Invoke([](auto& obj) { obj.OnShadowRender(); });
        ShadowMap_DrawEnd();
        SetUseShadowMap( 0, shadowMapDxLibHandle_) ;
        renderableCallbacks_.Invoke([](auto& obj) { obj.OnRender();       });
        SetUseShadowMap( 0, -1 );
    
        uiRenderableCallbacks_    .Invoke([](auto& obj) { obj.OnUserInterfaceRender(); });
        guiRenderableCallbacks_   .Invoke([](auto& obj) { obj.OnDebugRender();  });
        
        initRenderableCallbacks_  .OnUpdatePushedContents();
        awakableCallbacks_        .OnUpdatePushedContents();
        startableCallbacks_       .OnUpdatePushedContents();
        updatableCallbacks_       .OnUpdatePushedContents();
        lateUpdatableCallbacks_   .OnUpdatePushedContents();
        shadowRenderableCallbacks_.OnUpdatePushedContents();
        renderableCallbacks_      .OnUpdatePushedContents();
        uiRenderableCallbacks_    .OnUpdatePushedContents();
        guiRenderableCallbacks_   .OnUpdatePushedContents();
    }
}