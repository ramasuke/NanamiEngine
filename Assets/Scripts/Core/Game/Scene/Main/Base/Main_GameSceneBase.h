#pragma once
#include <concepts>

#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"
#include "../../../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../PlayerAvatar/RequireType/RequireType.h"
#include "../Context/Main_SceneContextBase.h"
#include "../Main_IGameScene.h"
#include "Context/Main_GameSceneBaseContext.h"

namespace GameCore::Scene::Main
{
    template<typename ContextT>
    requires std::derived_from<ContextT, SceneContextBase>
    class GameMainSceneBase : public IGameScene
    {
    public:
        explicit GameMainSceneBase(const std::weak_ptr<ContextT>& context, const GameSceneBaseContext& baseContext);
        virtual ~GameMainSceneBase() override = default;
        
    private:
        void Dispose() override;
        
        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        
    protected:
        /** template method pattern */
        virtual void DoDispose() = 0;
        
        /** @brief SandBox pattern */
        [[nodiscard]] std::shared_ptr<ContextT>   Context()                    const { return context_; }
        [[nodiscard]] GameProgresion              MainScenarioProgression()    const { return LoadGameProgression();   }
        [[nodiscard]] Sub::IGameSceneStack&       SubScene() const { return baseContext_.SubSceneStack(); }
        /**
         * @brief シーンをロードする
         * @warning 読み込んだシーンは自分で破棄する必要があります
         */
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> LoadMainScene() const;
        /**
         * @brief シーンをワーカースレッドで読み込み始める。完了までメインループは止まらない
         * @warning 読み込んだシーンは自分で破棄する必要があります
         */
        void BeginLoadMainSceneAsync() const;
        [[nodiscard]] bool IsMainSceneLoading() const;
        /** @brief BeginLoadMainSceneAsync で読み込んだシーン。完了前は空 */
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> LoadedMainScene() const;
        /** @brief 非同期ロードを始めて完了(または失敗)まで待つ。待っている間もフレームは回る */
        [[nodiscard]] Coroutine::Task<void> LoadMainSceneAsync() const;
        /** @brief 直近の非同期ロードが失敗していたか。Failed は一瞬で Idle に戻るのでフラグで見る */
        [[nodiscard]] bool HasMainSceneLoadFailed() const;
        /** @brief GameManage.scene に常駐しているロード画面 */
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return baseContext_.LoadingScreen(); }
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
        DoDispose();
        baseContext_.ClearSubScenes();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    std::weak_ptr<NanamiEngine::Scene::Scene> GameMainSceneBase<ContextT>::LoadMainScene() const
    {
        const auto scene = Context()->LoadSceneFile()->LoadScene();
        Core::Application::ApplicationBase::GameWindow()->AddContent(scene);
        Core::Application::ApplicationBase::GameWindow()->ChangeMainScene(scene);
        return scene;
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    void GameMainSceneBase<ContextT>::BeginLoadMainSceneAsync() const
    {
        Core::Application::ApplicationBase::GameWindow()->BeginLoadSceneAsync(Context()->LoadSceneFile()->GetContentPath());
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    bool GameMainSceneBase<ContextT>::IsMainSceneLoading() const
    {
        return Core::Application::ApplicationBase::GameWindow()->IsSceneLoading();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    std::weak_ptr<NanamiEngine::Scene::Scene> GameMainSceneBase<ContextT>::LoadedMainScene() const
    {
        return Core::Application::ApplicationBase::GameWindow()->LastAsyncLoadedScene();
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    Coroutine::Task<void> GameMainSceneBase<ContextT>::LoadMainSceneAsync() const
    {
        BeginLoadMainSceneAsync();
        co_await Coroutine::WaitUntil([]
        {
            return !Core::Application::ApplicationBase::GameWindow()->IsSceneLoading();
        });
    }

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    bool GameMainSceneBase<ContextT>::HasMainSceneLoadFailed() const
    {
        return Core::Application::ApplicationBase::GameWindow()->HasSceneLoadFailed();
    }
}
