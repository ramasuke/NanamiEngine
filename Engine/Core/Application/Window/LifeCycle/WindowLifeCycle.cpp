#include "WindowLifeCycle.h"

#include <algorithm>

#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "../../ApplicationBase.h"
#include "../../Configuration/ApplicationConfiguration.h"
#include "../../Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../../../Module/Asset/Asset.h"
#include "../../../Coroutine/Scheduler/CoroutineScheduler.h"
#include "../../../Physics/Physics.h"
#include "../../../../Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "../../Time/Time.h"

namespace NanamiEngine::Core::Application
{
    WindowLifeCycle::WindowLifeCycle(const bool useShadowMap)
        : coroutineScheduler_(std::make_unique<Coroutine::CoroutineScheduler>())
    {
        using Config = Configuration::AppConfiguration;

        if (useShadowMap)
        {
            const VECTOR lightDir = VGet(Config::GetLightDirX(), Config::GetLightDirY(), Config::GetLightDirZ());
            SetLightDirection(lightDir);
            const COLOR_F difColor = {Config::GetLightDifR(), Config::GetLightDifG(), Config::GetLightDifB(), 1.0f};
            SetLightDifColor(difColor);

            // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、直後の設定で完了待ちに入る
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            shadowMapDxLibHandle_ = MakeShadowMap(Config::GetShadowMapWidth(), Config::GetShadowMapHeight());
            SetUseASyncLoadFlag(useASyncLoad);
            SetShadowMapLightDirection(shadowMapDxLibHandle_, lightDir);
        }
    }

    WindowLifeCycle::~WindowLifeCycle() = default;

    void WindowLifeCycle::UpdateShadowMapDrawArea() const
    {
        if (shadowMapDxLibHandle_ == -1)
            return;

        // 範囲の手前側がカメラに来るよう、中心を前方へずらす
        const float  halfSize = Configuration::AppConfiguration::GetShadowAreaHalfSize();
        const VECTOR center   = VAdd(GetCameraPosition(), VScale(GetCameraFrontVector(), halfSize));
        const VECTOR extent   = VGet(halfSize, halfSize, halfSize);
        SetShadowMapDrawArea(shadowMapDxLibHandle_, VSub(center, extent), VAdd(center, extent));
    }

    void WindowLifeCycle::OnUpdateForGame()
    {
        // ロード中もフレームは回し続ける。暖機前のオブジェクトに OnUpdate が飛ばないよう、
        // 暖機と各グループへの追加反映を止め、コライダーが揃うまで物理も進めない
        const bool isLoadingResource = Module::Asset::Asset::IsLoadingResource();

        if (!isLoadingResource)
        {
            initRenderableCallbacks_  .Invoke([](auto& obj) { obj.InitRenderer();     });
            awakableCallbacks_        .Invoke([](auto& obj) { obj.OnAwake();          });
            ApplicationBase::Physics().Bodies().Flush();
            startableCallbacks_       .Invoke([](auto& obj) { obj.OnStart();          });
        }

        const auto fixedDeltaTime = 1.0f / static_cast<float>(Configuration::PhysicsConfiguration::GetFixedUpdateRate());
        Time::SetFixedDeltaTime(fixedDeltaTime);
        // 止めた暖機が済むまでは新しいシーンのコライダーが無く、先に Instantiate された Body だけが落ちて床を抜ける。
        // ロード明けにまとめて追いつかないよう、時間も貯めない
        if (isLoadingResource || hasDeferredPushedContents_)
        {
            accumulator_ = 0.0f;
        }
        else
        {
            const float rawDeltaTime = Time::DeltaTime();
            const float deltaTime = (std::min)(rawDeltaTime, Configuration::PhysicsConfiguration::GetMaxDeltaTime());
            if (rawDeltaTime > 0.0f)
            {
                accumulator_ += deltaTime;
            }
            const int   maxStep         = Configuration::PhysicsConfiguration::GetMaxPhysicsStep();
            const float maxAccumulation = fixedDeltaTime * static_cast<float>(maxStep);
            accumulator_ = (std::min)(accumulator_, maxAccumulation);
            int step = 0;
            while (accumulator_ >= fixedDeltaTime && step < maxStep)
            {
                preFixedUpdateCallbacks_.Invoke([](auto& obj) { obj.OnPreFixedUpdate(); });
                fixedUpdatableCallbacks_.Invoke([](auto& obj) { obj.OnFixedUpdate(); });
                // 物理ボディを動かす待機(WaitForTweenBody など)。ここで積んだ速度指示は直後の OnBeginPhysics で反映される
                coroutineScheduler_->InvokeFixed(fixedDeltaTime);
                ApplicationBase::Physics().Bodies().Flush();
                beginPhysicsCallbacks_  .Invoke([](auto& obj) { obj.OnBeginPhysics(); });
                ApplicationBase::Physics().Update(fixedDeltaTime);
                endPhysicsCallbacks_    .Invoke([](auto& obj) { obj.OnUpdatedPhysics(); });

                accumulator_ -= fixedDeltaTime;
                step++;
            }
        }
        Time::SetFixedAlpha(accumulator_ / fixedDeltaTime);
        
        updatableCallbacks_       .Invoke([](auto& obj) { obj.OnUpdate();         });
        lateUpdatableCallbacks_   .Invoke([](auto& obj) { obj.OnLateUpdate();     });
        coroutineScheduler_      ->Invoke();

        UpdateShadowMapDrawArea();
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

        // フレームの途中で要求されたロード(同期のシーン読み込みなど)でも止めるため、ここで取り直す。
        // ロード明けの最初のフレームは反映がフレーム末なので、その物理も hasDeferredPushedContents_ で止める
        if (Module::Asset::Asset::IsLoadingResource())
        {
            hasDeferredPushedContents_ = true;
        }
        else
        {
            hasDeferredPushedContents_ = false;
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
    }
    
    void WindowLifeCycle::OnUpdateForEditor()
    {
        initRenderableCallbacks_  .Invoke([](auto& obj) { obj.InitRenderer();   });

        UpdateShadowMapDrawArea();
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
