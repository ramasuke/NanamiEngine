#include "Main_GameSceneBaseContext.h"

#include "../../../Sub/Group/Sub_IGameSceneGroup.h"

namespace GameCore::Scene::Main
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
        const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen)
        : subSceneStack_(subSceneStack)
        , loadingScreen_(loadingScreen)
    {
        
    }

    void GameSceneBaseContext::ClearSubScenes() const
    {
        subSceneStack_->Clear();
    }
}
