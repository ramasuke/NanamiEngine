#include "WindowLifeCycle.h"

#include <algorithm>

#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "../../ApplicationBase.h"
#include "../../../../Module/Asset/Asset.h"
#include "../../../../Module/Scene/ShadowMap/ShadowMapSetting.h"
#include "../../../Coroutine/Scheduler/CoroutineScheduler.h"
#include "../../../Physics/Physics.h"
#include "../../Time/Time.h"

namespace NanamiEngine::Core::Application
{
    WindowLifeCycle::WindowLifeCycle(const bool useShadowMap)
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
        , fixedDeltaTime_(1.0f / 180.0f)
    {
        if (useShadowMap)
        {
            const VECTOR lightDir = VGet(-0.5f, -1.0f, -0.5f);
            SetLightDirection(lightDir);
            constexpr COLOR_F difColor = {1.0f, 1.0f, 1.0f, 1.0f};
            SetLightDifColor(difColor);
            
            shadowMapDxLibHandle_ = MakeShadowMap(1024, 1024);
            SetShadowMapLightDirection(shadowMapDxLibHandle_, VGet(-0.5f, -1.0f, -0.5f));
            const glm::vec3 position = Scene::ShadowMapSetting::GetRenderAreaPos();
            const glm::vec3 size     = Scene::ShadowMapSetting::GetRenderAreaSize();
            const VECTOR minPosition = VGet(position.x + -size.x, position.y + -size.y, position.z + -size.z);
            const VECTOR maxPosition = VGet(position.x + size.x, position.y + size.y, position.z + size.z);
            SetShadowMapDrawArea(shadowMapDxLibHandle_, minPosition, maxPosition);
        }
    }
    
    WindowLifeCycle::~WindowLifeCycle() = default;
    
    void WindowLifeCycle::OnUpdateForGame()
    {
        if (Module::Asset::Asset::IsLoadingResource())
            return;
        
        initRenderableCallbacks_  .Invoke([](auto& obj) { obj.InitRenderer();     });
        awakableCallbacks_        .Invoke([](auto& obj) { obj.OnAwake();          });
        startableCallbacks_       .Invoke([](auto& obj) { obj.OnStart();          });

        const float rawDeltaTime = Time::DeltaTime();

        // 異常なフレーム時間を制限
        const float deltaTime = (std::min)(rawDeltaTime, 0.2f);
        if (rawDeltaTime > 0.0f)
        {
            accumulator_ += deltaTime;
        }
        //無限蓄積防止
        const float maxAccumulation = fixedDeltaTime_ * 2.0f;
        accumulator_ = (std::min)(accumulator_, maxAccumulation);
        constexpr int maxStep = 2;
        int step = 0;
        while (accumulator_ >= fixedDeltaTime_ && step < maxStep)
        {
            beginPhysicsCallbacks_.Invoke([](auto& obj) { obj.OnBeginPhysics(); });
            ApplicationBase::Physics().Update(fixedDeltaTime_);
            endPhysicsCallbacks_.Invoke([](auto& obj) { obj.OnUpdatedPhysics(); });

            accumulator_ -= fixedDeltaTime_;
            step++;
        }
        
        updatableCallbacks_       .Invoke([](auto& obj) { obj.OnUpdate();         });
        lateUpdatableCallbacks_   .Invoke([](auto& obj) { obj.OnLateUpdate();     });
        coroutineScheduler_      ->Invoke();

        ShadowMap_DrawSetup(shadowMapDxLibHandle_);
        shadowRenderableCallbacks_.Invoke([](auto& obj) { obj.OnShadowRender();   });
        ShadowMap_DrawEnd();
        SetUseShadowMap( 0, shadowMapDxLibHandle_) ;
        renderableCallbacks_      .Invoke([](auto& obj) { obj.OnRender();         });
        SetUseShadowMap( 0, -1);

        Effekseer_Sync3DSetting();
        UpdateEffekseer3D();
        DrawEffekseer3D();
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

        Effekseer_Sync3DSetting();
        UpdateEffekseer3D();
        DrawEffekseer3D();
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

    void WindowLifeCycle::InitRenderableAddedContentPop()
    {
        initRenderableCallbacks_.AddedContentPop();
    }

    void WindowLifeCycle::AwakableAddedContentPop()
    {
        awakableCallbacks_.AddedContentPop();
    }

    void WindowLifeCycle::StartableAddedContentPop()
    {
        startableCallbacks_.AddedContentPop();        
    }
}
