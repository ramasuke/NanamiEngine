#pragma once
#include <concepts>

#include "../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../PlayerAvatar/RequireType/RequireType.h"
#include "../../../PlayerAvatar/StateMachine/PlayerAvatarStateMachine.h"
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
        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        
    protected:
        /** @brief 以下からサンドボックスパターン */
        [[nodiscard]] std::shared_ptr<ContextT>   Context()                    const { return context_;                              }
        [[nodiscard]] MainScenarioProgression     GetMainScenarioProgression() const { return baseContext_.MainScenarioProgression(); }
        [[nodiscard]] Sub::IGameSceneStack&       SubScene() const { return baseContext_.SubSceneStack(); }
        /**
         * @brief シーンをロードする
         * @warning 読み込んだシーンは自分で破棄する必要があります
         */
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> LoadMainScene() const;
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
    std::weak_ptr<NanamiEngine::Scene::Scene> GameMainSceneBase<ContextT>::LoadMainScene() const
    {
        const auto scene = Context()->LoadSceneFile()->LoadScene();
        Core::Application::ApplicationBase::GameWindow()->AddContent(scene);
        Core::Application::ApplicationBase::GameWindow()->ChangeMainScene(scene);
        return scene;
    }
}
