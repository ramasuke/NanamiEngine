#pragma once
#include <concepts>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/LoadScene/Coroutine_LoadSceneAsync.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Packages/R4/R4.h"
#include "../../../PlayerAvatar/RequireType/RequireType.h"
#include "../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../Context/Main_SceneContextBase.h"
#include "../Loading/Main_SceneLoadStep.h"
#include "../Main_IGameScene.h"
#include "Context/Main_GameSceneBaseContext.h"

namespace GameCore::Scene::Main
{
    // NOTE: OnEnterAsync の結果
    struct EnterResult final
    {
        bool        succeeded = false;
        std::string failure;

        [[nodiscard]] static EnterResult Ok() { return { true, {} }; }
        [[nodiscard]] static EnterResult Fail(std::string message) { return { false, std::move(message) }; }
        [[nodiscard]] explicit operator bool() const { return succeeded; }
    };

    // NOTE: メインシーンの基底。入場に失敗したら FallbackSceneOnFailure へ逃がす
    template<typename ContextT>
    requires std::derived_from<ContextT, SceneContextBase>
    class GameMainSceneBase : public IGameScene
    {
    public:
        explicit GameMainSceneBase(const std::weak_ptr<ContextT>& context, const GameSceneBaseContext& baseContext);
        virtual ~GameMainSceneBase() override = default;

        void Init() final;

    private:
        void Exit() override;
        void Dispose() override;
        void CancelEnter();
        // NOTE: 読み込んだメインシーンとサブシーンを外す
        void Unload();
        [[nodiscard]] bool IsEntered() const override { return isEntered_; }
        [[nodiscard]] std::shared_ptr<SceneContextBase> BaseContext() const override { return context_; }
        Coroutine::Task<void> EnterAsync(NanamiEngine::R4::CancellationToken token);
        // NOTE: 入場の失敗をロード画面に出しセーフ処理
        void FailEnter(const NanamiEngine::R4::CancellationToken& token, const std::string& message);
        // NOTE: 失敗の表示を少し見せてからセーフ処理
        Coroutine::Task<void> FallbackAfterFailureAsync(NanamiEngine::R4::CancellationToken token, SceneType fallback);

        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        std::weak_ptr<NanamiEngine::Scene::Scene> mainScene_;

        std::optional<NanamiEngine::R4::CancellationTokenSource> enterCancellation_;
        bool isEntered_ = false;

    protected:
        // NOTE: 派生シーンが埋める部分（テンプレートメソッドパターン）
        virtual void DoExit() = 0;
        virtual void OnInit() {}
        [[nodiscard]] virtual std::vector<Sub::SceneType> SubScenes() const = 0;
        
        virtual Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) = 0;
        virtual void OnEntered() = 0;
        
        // NOTE: 入場に失敗したときの逃がし先。空なら逃がさずそのまま
        [[nodiscard]] virtual std::optional<SceneType> FallbackSceneOnFailure() const { return SceneType::MainIsland; }

        // NOTE: 派生シーンに渡す部品の窓口（サンドボックスパターン）
        [[nodiscard]] std::shared_ptr<ContextT>   Context()                    const { return context_; }
        [[nodiscard]] GameProgresion              MainScenarioProgression()    const { return LoadGameProgression();   }
        [[nodiscard]] Sub::IGameSceneStack&       SubScene() const { return baseContext_.SubSceneStack(); }
        // NOTE: GameManage.scene と一緒に常駐しているロード画面
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return baseContext_.LoadingScreen(); }
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> MainScene() const { return mainScene_; }
    };

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    GameMainSceneBase<ContextT>::GameMainSceneBase(
          const std::weak_ptr<ContextT>& context
        , const GameSceneBaseContext& baseContext)
        : context_    (context.lock())
        , baseContext_(baseContext   )
    {

    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::Init()
    {
        if (enterCancellation_)
        {
            enterCancellation_->Cancel();
        }
        enterCancellation_.emplace();
        isEntered_ = false;

        OnInit();
        Coroutine::StartCoroutine(EnterAsync(enterCancellation_->Token()));
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::Exit()
    {
        CancelEnter();
        DoExit();
        Unload();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::Dispose()
    {
        CancelEnter();
        Unload();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::CancelEnter()
    {
        if (enterCancellation_)
        {
            enterCancellation_->Cancel();
        }
        isEntered_ = false;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::Unload()
    {
        if (const auto scene = mainScene_.lock())
            Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene);
        mainScene_.reset();

        baseContext_.ClearSubScenes();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<void> GameMainSceneBase<ContextT>::EnterAsync(const NanamiEngine::R4::CancellationToken token)
    {
        LoadingScreen().SetStep(SceneLoadStep::Deserializing);
        const auto loaded = co_await Coroutine::LoadSceneAsync(Context()->LoadSceneFile()->GetContentPath(), token);
        if (token.IsCancellationRequested())
            co_return;
        
        if (!loaded)
        {
            FailEnter(token, "ステージの読み込みに失敗しました");
            co_return;
        }
        mainScene_ = loaded.scene;
        LoadingScreen().SetStep(SceneLoadStep::Warmup);
        
        for (const auto type : SubScenes())
        {
            co_await SubScene().PushAsync(type);
            
            if (token.IsCancellationRequested())
                co_return;
        }

        const EnterResult result = co_await OnEnterAsync(token);
        if (token.IsCancellationRequested())
            co_return;
        
        if (!result)
        {
            FailEnter(token, result.failure.empty() ? std::string("入場に失敗しました") : result.failure);
            co_return;
        }

        LoadingScreen().SetStep(SceneLoadStep::Completed);
        isEntered_ = true;
        OnEntered();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::FailEnter(const NanamiEngine::R4::CancellationToken& token, const std::string& message)
    {
        if (token.IsCancellationRequested())
            return;

        LoadingScreen().Fail(message);

        if (const auto fallback = FallbackSceneOnFailure())
        {
            Coroutine::StartCoroutine(FallbackAfterFailureAsync(token, *fallback));
            return;
        }

        isEntered_ = true;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<void> GameMainSceneBase<ContextT>::FallbackAfterFailureAsync(
        const NanamiEngine::R4::CancellationToken token, 
        const SceneType fallback)
    {
        const int startedMs = Time::NowMilliseconds();
        co_await Coroutine::WaitUntil([startedMs] { return Time::NowMilliseconds() - startedMs >= 2000; });
        if (token.IsCancellationRequested())
            co_return;

        baseContext_.RequestChangeScene(fallback);
    }
}
