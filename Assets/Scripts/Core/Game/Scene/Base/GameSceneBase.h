#pragma once
#include <concepts>

#include "../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../PlayerAvatar/RequireType/RequireType.h"
#include "../../PlayerAvatar/StateMachine/PlayerAvatarStateMachine.h"
#include "../Context/SceneContextBase.h"
#include "../IGameScene.h"
#include "Context/GameSceneBaseContext.h"

namespace GameCore::Scene
{
    template<typename ContextT>
    requires std::derived_from<ContextT, SceneContextBase>
    class GameSceneBase : public IGameScene
    {
    public:
        explicit GameSceneBase(const std::weak_ptr<ContextT>& context, const GameSceneBaseContext& baseContext);
        virtual ~GameSceneBase() override = default;
        
    private:
        std::shared_ptr<ContextT> context_;
        GameSceneBaseContext      baseContext_;
        
    protected:
        /** @brief 以下からサンドボックスパターン */
        [[nodiscard]] std::shared_ptr<ContextT> Context()                    const { return context_;                              }
        [[nodiscard]] MainScenarioProgression   GetMainScenarioProgression() const { return *baseContext_.mainScenarioProgression_; }
        /**
         * @brief シーンをロードする
         * @warning 読み込んだシーンは自分で破棄する必要がある
         */
        [[nodiscard]] std::weak_ptr<NanamiEngine::Scene::Scene> OnLoadMainScene() const;
    };

    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    GameSceneBase<ContextT>::GameSceneBase(
          const std::weak_ptr<ContextT>& context
        , const GameSceneBaseContext& baseContext)
        : context_    (context.lock())
        , baseContext_(baseContext   )
    {
        
    }
    
    template <typename ContextT> requires std::derived_from<ContextT, SceneContextBase>
    std::weak_ptr<NanamiEngine::Scene::Scene> GameSceneBase<ContextT>::OnLoadMainScene() const
    {
        const auto scene = Context()->LoadSceneFile()->LoadScene();
        Core::Application::ApplicationBase::GameWindow()->AddContent(scene);
        Core::Application::ApplicationBase::GameWindow()->ChangeMainScene(scene);
        return scene;
    }
}
