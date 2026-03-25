#include "GameSceneBaseContext.h"

namespace GameCore::Scene
{
    GameSceneBaseContext::GameSceneBaseContext(
        const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression)
        : mainScenarioProgression_(mainScenarioProgression)
    {
        
    }
}
