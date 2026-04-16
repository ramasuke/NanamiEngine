#pragma once
#include <memory>
#include <type_traits>

#include "../Sub_IGameScene.h"
#include "../../../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../Context/Sub_SceneContextBase.h"

namespace GameCore::Scene::Sub
{
    template<typename SceneContextT>
    requires std::is_base_of_v<SceneContextBase, SceneContextT>
    class GameSceneBase : public IGameScene
    {
    public:
        explicit GameSceneBase(const std::shared_ptr<SceneContextT>& sceneContext);
        virtual ~GameSceneBase() = default;
        void Init() final
        {
            DoInit();
        }
        void Dispose() final
        {
            DoDispose();
        }
        void OnDrawGui() final
        {
            DoDrawGui();
        }

    private:
        const std::weak_ptr<SceneContextT> sceneContext_;
        
    protected:
        virtual void DoInit   () = 0;
        virtual void DoDispose() = 0;
        virtual void DoDrawGui() = 0;
        std::weak_ptr<NanamiEngine::Scene::Scene> LoadScene();
        [[nodiscard]] SceneContextT& SceneContext() const { return *sceneContext_.lock(); }
    };

    template <typename SceneContextT>
    requires std::is_base_of_v<SceneContextBase, SceneContextT>
    GameSceneBase<SceneContextT>::GameSceneBase(const std::shared_ptr<SceneContextT>& sceneContext)
        : sceneContext_(sceneContext)
    {
        
    }

    template <typename SceneContextT> requires std::is_base_of_v<SceneContextBase, SceneContextT>
    std::weak_ptr<NanamiEngine::Scene::Scene> GameSceneBase<SceneContextT>::LoadScene()
    {
        const auto scene = SceneContext().GetSceneFile().LoadScene();
        Core::Application::ApplicationBase::GameWindow()->AddContent(scene);
        return scene;
    }
}
