#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    class AboardAirShipMovie;
}

namespace GameCore::Scene::Main
{
    class FirstTouchDownMainIsLandScene final : public GameMainSceneBase<FirstTouchDownMainIsLandSceneContext>
    {
    public:
        using ContextT = FirstTouchDownMainIsLandSceneContext;
        explicit FirstTouchDownMainIsLandScene(const std::weak_ptr<FirstTouchDownMainIsLandSceneContext>& context, GameSceneBaseContext baseContext);
        ~FirstTouchDownMainIsLandScene() override;
        
    private:
        void Init     () override;
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;

        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::unique_ptr<FirstTouchDownMainIsLand::AboardAirShipMovie> aboardAirShipMovie_;
        std::weak_ptr<GameObject::IGameObject> playerStatusPresenter_;
    };
}
