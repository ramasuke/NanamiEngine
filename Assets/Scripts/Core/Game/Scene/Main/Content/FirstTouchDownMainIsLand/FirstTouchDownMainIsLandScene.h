#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class StatusPresenter;
}

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
        void OnEnter  () override;
        void OnExit   () override;
        void Dispose  () override;
        void OnDrawGui() override;

        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> playerAvatar_;
        std::unique_ptr<FirstTouchDownMainIsLand::AboardAirShipMovie> aboardAirShipMovie_;
        std::unique_ptr<PlayerAvatar::SwordMan::StatusPresenter> playerStatusPresenter_;
    };
}
