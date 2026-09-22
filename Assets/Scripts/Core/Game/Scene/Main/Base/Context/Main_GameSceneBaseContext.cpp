#include "Main_GameSceneBaseContext.h"

#include "../../../Sub/Group/Sub_IGameSceneGroup.h"

namespace GameCore::Scene::Main
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
        const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen,
        RequestChangeSceneFunc requestChangeScene)
        : subSceneStack_(subSceneStack)
        , loadingScreen_(loadingScreen)
        , requestChangeScene_(std::move(requestChangeScene))
    {
        
    }

    void GameSceneBaseContext::ClearSubScenes() const
    {
        subSceneStack_->Clear();
    }
}
