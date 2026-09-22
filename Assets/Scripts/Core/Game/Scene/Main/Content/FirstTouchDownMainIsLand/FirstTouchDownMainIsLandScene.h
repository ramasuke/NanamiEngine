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
        explicit FirstTouchDownMainIsLandScene(const std::weak_ptr<FirstTouchDownMainIsLandSceneContext>& context, GameSceneBaseContext baseContext);
        ~FirstTouchDownMainIsLandScene() override;
        
    private:
        void Init     () override;
        Coroutine::Task<void> OnEnterAsync(int generation);
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        /** @brief 導入が読めなければタイトルへ戻す */
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return SceneType::Title; }

        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::shared_ptr<FirstTouchDownMainIsLand::AboardAirShipMovie> aboardAirShipMovie_;
        std::weak_ptr<GameObject::IGameObject> playerStatusPresenter_;
    };
}
