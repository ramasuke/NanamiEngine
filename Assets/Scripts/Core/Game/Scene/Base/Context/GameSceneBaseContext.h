#pragma once
#include <memory>

#include "../../../MainProgression/MainProgression.h"

namespace GameCore::Scene
{
    class GameSceneBaseContext final
    {
    public:
        explicit GameSceneBaseContext(const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression);
        
        std::shared_ptr<MainScenarioProgression> mainScenarioProgression_;
    };
}
