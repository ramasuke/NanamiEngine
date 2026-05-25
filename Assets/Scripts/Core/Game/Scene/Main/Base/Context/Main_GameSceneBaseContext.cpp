#include "Main_GameSceneBaseContext.h"

namespace GameCore::Scene::Main
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : subSceneStack_(subSceneStack)
    {
        
    }
}
