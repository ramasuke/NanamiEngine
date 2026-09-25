#pragma once
#include <concepts>
#include <optional>
#include <string>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../PlayerAvatar/RequireType/RequireType.h"
#include "../Context/Main_SceneContextBase.h"
#include "../Loading/Main_SceneLoadStep.h"
#include "../Main_IGameScene.h"
#include "Context/Main_GameSceneBaseContext.h"

namespace GameCore::Scene::Main
{
    /**
     * @brief メインシーンの基底。
     *
     * 入場は Init で始めたコルーチンの中で進める。シーンファイルは非同期で読み込み、
     * 読み込みが済むまでメインシーンが居ないので、その間は Instantiate(プレイヤー生成・SubScene の Push)をしない。
     * 準備が済んだら CompleteEnter を呼ぶ。ロード画面の表示と片付けは GameSceneGroup が受け持つ
     */
    template<typename ContextT>
    requires std::derived_from<ContextT, SceneContextBase>
    class GameMainSceneBase : public IGameScene
    {
    public:
        explicit GameMainSceneBase(const std::weak_ptr<ContextT>& context, const GameSceneBaseContext& baseContext);
        virtual ~GameMainSceneBase() override = default;

    private:
        void Dispose() override;
        [[nodiscard]] bool IsEntered() const override { return isEntered_; }
        /** @brief 失敗の表示を少し見せてから、逃げ先への遷移を頼む */
        Coroutine::Task<void> FallbackAfterFailureAsync(int generation, SceneType fallback);

        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        std::weak_ptr<NanamiEngine::Scene::Scene> mainScene_;
        int  enterGeneration_ = 0;
        bool isEntered_       = false;

    protected:
        /** template method pattern */
        virtual void DoDispose() = 0;
        /** @brief 入場に失敗したときの逃げ先。nullopt なら逃がさずにロード画面を明ける */
        [[nodiscard]] virtual std::optional<SceneType> FallbackSceneOnFailure() const { return SceneType::MainIsland; }

        /** @brief SandBox pattern */
        [[nodiscard]] std::shared_ptr<ContextT>   Context()                    const { return context_; }
        [[nodiscard]] GameProgresion              MainScenarioProgression()    const { return LoadGameProgression();   }
        [[nodiscard]] Sub::IGameSceneStack&       SubScene() const { return baseContext_.SubSceneStack(); }
        /** @brief GameManage.scene と一緒に常駐しているロード画面 */
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return baseContext_.LoadingScreen(); }
        /** @brief LoadMainSceneAsync で読み込んだシーン。Dispose で自動的に外す */
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> MainScene() const { return mainScene_; }

        /**
         * @brief Init の頭で呼ぶ。前回の入場コルーチンを無効にして、今回の世代番号を返す
         * @note コルーチンは止められないので、co_await から戻るたびに IsCurrentEnter で確かめる
         */
        int BeginEnter();
        [[nodiscard]] bool IsCurrentEnter(const int generation) const { return generation == enterGeneration_; }
        /**
         * @brief メインシーンを非同期で読み込み、AddContent まで済ませる。待っている間もフレームは回る
         * @return 読み込めたら true。失敗したとき(逃げ先へ遷移を頼み済み)と、別の入場に追い越されたときは false
         */
        [[nodiscard]] Coroutine::Task<bool> LoadMainSceneAsync(int generation);
        /** @brief 入場の失敗をロード画面に出し、逃げ先への遷移を頼む */
        void FailEnter(int generation, const std::string& message);
        /** @brief 入場の準備が済んだことを知らせる */
        void CompleteEnter(int generation);
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
    void GameMainSceneBase<ContextT>::Dispose()
    {
        // 走っている入場コルーチンを無効化する
        ++enterGeneration_;
        isEntered_ = false;

        DoDispose();

        if (const auto scene = mainScene_.lock())
            Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene);
        mainScene_.reset();

        baseContext_.ClearSubScenes();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    int GameMainSceneBase<ContextT>::BeginEnter()
    {
        isEntered_ = false;
        return ++enterGeneration_;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<bool> GameMainSceneBase<ContextT>::LoadMainSceneAsync(const int generation)
    {
        LoadingScreen().SetStep(SceneLoadStep::Deserializing);

        const auto gameWindow = Core::Application::ApplicationBase::GameWindow();
        if (!gameWindow->BeginLoadSceneAsync(Context()->LoadSceneFile()->GetContentPath()))
        {
            FailEnter(generation, "ステージの読み込みを始められませんでした");
            co_return false;
        }

        co_await Coroutine::WaitUntil([gameWindow] { return !gameWindow->IsSceneLoading(); });
        if (!IsCurrentEnter(generation))
            co_return false;

        const auto scene = gameWindow->LastAsyncLoadedScene().lock();
        if (gameWindow->HasSceneLoadFailed() || !scene)
        {
            FailEnter(generation, "ステージの読み込みに失敗しました");
            co_return false;
        }

        mainScene_ = scene;
        LoadingScreen().SetStep(SceneLoadStep::Warmup);
        co_return true;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::FailEnter(const int generation, const std::string& message)
    {
        if (!IsCurrentEnter(generation))
            return;

        LoadingScreen().Fail(message);

        if (const auto fallback = FallbackSceneOnFailure())
        {
            Coroutine::StartCoroutine(FallbackAfterFailureAsync(generation, *fallback));
            return;
        }

        // 逃げ先が無ければ、壊れたままでも画面は明ける
        isEntered_ = true;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<void> GameMainSceneBase<ContextT>::FallbackAfterFailureAsync(const int generation, const SceneType fallback)
    {
        // ロード中は DeltaTime が止まるので、壁時計で待つ
        const int startedMs = Time::NowMilliseconds();
        co_await Coroutine::WaitUntil([startedMs] { return Time::NowMilliseconds() - startedMs >= 2000; });
        if (!IsCurrentEnter(generation))
            co_return;

        baseContext_.RequestChangeScene(fallback);
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::CompleteEnter(const int generation)
    {
        if (!IsCurrentEnter(generation))
            return;

        LoadingScreen().SetStep(SceneLoadStep::Completed);
        isEntered_ = true;
    }
}
