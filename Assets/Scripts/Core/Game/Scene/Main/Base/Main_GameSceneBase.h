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
    /** @brief OnEnterAsync の結果。既定値は失敗なので、コルーチン内の例外で既定値が返っても成功扱いにならない */
    struct EnterResult final
    {
        bool        succeeded = false;
        /** 失敗したときにロード画面へ出す文言。空なら汎用の文言 */
        std::string failure;

        [[nodiscard]] static EnterResult Ok() { return { true, {} }; }
        [[nodiscard]] static EnterResult Fail(std::string message) { return { false, std::move(message) }; }
        [[nodiscard]] explicit operator bool() const { return succeeded; }
    };

    /**
     * @brief メインシーンの基底。
     *
     * 入場の流れは基底が持つ: シーンファイルを読み込む → SubScenes を積む → OnEnterAsync → OnEntered。
     * どこかで失敗したらロード画面に出して FallbackSceneOnFailure へ逃がす。
     * 入場の途中で抜けたら(Dispose / 次の Init)token がキャンセルされるので、OnEnterAsync は co_await のあとで確かめる。
     * ロード画面の表示と片付けは GameSceneGroup が受け持つ
     */
    template<typename ContextT>
    requires std::derived_from<ContextT, SceneContextBase>
    class GameMainSceneBase : public IGameScene
    {
    public:
        explicit GameMainSceneBase(const std::weak_ptr<ContextT>& context, const GameSceneBaseContext& baseContext);
        virtual ~GameMainSceneBase() override = default;

        void Init() final;

    private:
        void Dispose() override;
        [[nodiscard]] bool IsEntered() const override { return isEntered_; }
        Coroutine::Task<void> EnterAsync(NanamiEngine::R4::CancellationToken token);
        /** @brief 入場の失敗をロード画面に出し、逃げ先への遷移を頼む */
        void FailEnter(const NanamiEngine::R4::CancellationToken& token, const std::string& message);
        /** @brief 失敗の表示を少し見せてから、逃げ先への遷移を頼む */
        Coroutine::Task<void> FallbackAfterFailureAsync(NanamiEngine::R4::CancellationToken token, SceneType fallback);

        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        std::weak_ptr<NanamiEngine::Scene::Scene> mainScene_;
        /** @note CancellationTokenSource は代入しても作り直されないので、入場ごとに emplace する */
        std::optional<NanamiEngine::R4::CancellationTokenSource> enterCancellation_;
        bool isEntered_ = false;

    protected:
        /** template method pattern */
        virtual void DoDispose() = 0;
        /** @brief 読み込みを始める前に同期で呼ぶ。読み込んだシーン内の FIELD にはまだ触れない */
        virtual void OnInit() {}
        /** @brief メインシーンを読み込んだあとに積むサブシーン */
        [[nodiscard]] virtual std::vector<Sub::SceneType> SubScenes() const { return {}; }
        /**
         * @brief メインシーンとサブシーンが揃ってから呼ぶ入場処理。ロード画面はまだ覆っている
         * @note co_await から戻ったら token.IsCancellationRequested() を確かめる(コルーチンは止められない)
         */
        virtual Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) = 0;
        /** @brief 入場が済み、ロード画面が明け始めるときに呼ぶ */
        virtual void OnEntered() {}
        /** @brief 入場に失敗したときの逃げ先。nullopt なら逃がさずにロード画面を明ける */
        [[nodiscard]] virtual std::optional<SceneType> FallbackSceneOnFailure() const { return SceneType::MainIsland; }

        /** @brief SandBox pattern */
        [[nodiscard]] std::shared_ptr<ContextT>   Context()                    const { return context_; }
        [[nodiscard]] GameProgresion              MainScenarioProgression()    const { return LoadGameProgression();   }
        [[nodiscard]] Sub::IGameSceneStack&       SubScene() const { return baseContext_.SubSceneStack(); }
        /** @brief GameManage.scene と一緒に常駐しているロード画面 */
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return baseContext_.LoadingScreen(); }
        /** @brief 入場で読み込んだシーン。Dispose で自動的に外す */
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
        // 前回の入場コルーチンが残っていれば、次の co_await 明けで抜けさせる
        if (enterCancellation_)
            enterCancellation_->Cancel();
        enterCancellation_.emplace();
        isEntered_ = false;

        OnInit();
        Coroutine::StartCoroutine(EnterAsync(enterCancellation_->Token()));
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::Dispose()
    {
        // 走っている入場コルーチンを無効化する
        if (enterCancellation_)
            enterCancellation_->Cancel();
        isEntered_ = false;

        DoDispose();

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

        // メインシーンが揃ってから積むので、サブシーンの Instantiate はメインシーンへ入る
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

        // 逃げ先が無ければ、壊れたままでも画面は明ける
        isEntered_ = true;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<void> GameMainSceneBase<ContextT>::FallbackAfterFailureAsync(const NanamiEngine::R4::CancellationToken token, const SceneType fallback)
    {
        // ロード中は DeltaTime が止まるので、壁時計で待つ
        const int startedMs = Time::NowMilliseconds();
        co_await Coroutine::WaitUntil([startedMs] { return Time::NowMilliseconds() - startedMs >= 2000; });
        if (token.IsCancellationRequested())
            co_return;

        baseContext_.RequestChangeScene(fallback);
    }
}
