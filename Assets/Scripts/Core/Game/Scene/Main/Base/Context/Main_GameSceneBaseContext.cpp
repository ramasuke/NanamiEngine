#include "Main_GameSceneBaseContext.h"

namespace GameCore::Scene::Main
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<GameCore::MainScenarioProgression>& mainScenarioProgression,
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : mainScenarioProgression_(mainScenarioProgression)
        , subSceneStack_(subSceneStack)
    {
        
    }
}
