#include "WindowLifeCycle.h"

#include <algorithm>

#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "../../ApplicationBase.h"
#include "../../Configuration/ApplicationConfiguration.h"
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
    {
        using Cfg = Configuration::AppConfiguration;
        fixedDeltaTime_ = 1.0f / static_cast<float>(Cfg::GetFixedUpdateRate());

        if (useShadowMap)
        {
            const VECTOR lightDir = VGet(Cfg::GetLightDirX(), Cfg::GetLightDirY(), Cfg::GetLightDirZ());
            SetLightDirection(lightDir);
            const COLOR_F difColor = {Cfg::GetLightDifR(), Cfg::GetLightDifG(), Cfg::GetLightDifB(), 1.0f};
            SetLightDifColor(difColor);

            shadowMapDxLibHandle_ = MakeShadowMap(Cfg::GetShadowMapWidth(), Cfg::GetShadowMapHeight());
            SetShadowMapLightDirection(shadowMapDxLibHandle_, lightDir);
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
        const float deltaTime = (std::min)(rawDeltaTime, Configuration::AppConfiguration::GetMaxDeltaTime());
        if (rawDeltaTime > 0.0f)
        {
            accumulator_ += deltaTime;
        }
        const int   maxStep         = Configuration::AppConfiguration::GetMaxPhysicsStep();
        const float maxAccumulation = fixedDeltaTime_ * static_cast<float>(maxStep);
        accumulator_ = (std::min)(accumulator_, maxAccumulation);
        int step = 0;
        while (accumulator_ >= fixedDeltaTime_ && step < maxStep)
        {
            preFixedUpdateCallbacks_.Invoke([](auto& obj) { obj.OnPreFixedUpdate(); });
            fixedUpdatableCallbacks_.Invoke([](auto& obj) { obj.OnFixedUpdate(); });
            beginPhysicsCallbacks_  .Invoke([](auto& obj) { obj.OnBeginPhysics(); });
            ApplicationBase::Physics().Update(fixedDeltaTime_);
            endPhysicsCallbacks_    .Invoke([](auto& obj) { obj.OnUpdatedPhysics(); });

            accumulator_ -= fixedDeltaTime_;
            step++;
        }
        Time::SetFixedAlpha(accumulator_ / fixedDeltaTime_);
        
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
        fixedUpdatableCallbacks_  .OnUpdatePushedContents();
        preFixedUpdateCallbacks_  .OnUpdatePushedContents();
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
        fixedUpdatableCallbacks_  .OnUpdatePushedContents();
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
