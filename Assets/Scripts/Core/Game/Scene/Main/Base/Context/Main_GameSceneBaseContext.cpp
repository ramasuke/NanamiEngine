#include "Main_GameSceneBaseContext.h"

#include "../../../Sub/Group/Sub_IGameSceneGroup.h"

namespace GameCore::Scene::Main
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : subSceneStack_(subSceneStack)
    {
        
    }

    void GameSceneBaseContext::ClearSubScenes() const
    {
        subSceneStack_->Clear();
    }
}
