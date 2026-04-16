#pragma once
#include <memory>

#include "../../../../MainProgression/MainProgression.h"

namespace GameCore::Scene::Sub
{
    class IGameSceneStack;
}

namespace GameCore::Scene::Main
{
    class GameSceneBaseContext final
    {
    public:
        explicit GameSceneBaseContext(
            const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression,
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack);

        [[nodiscard]] MainScenarioProgression& MainScenarioProgression() const { return *mainScenarioProgression_; }
        [[nodiscard]] Sub::IGameSceneStack& SubSceneStack() const { return *subSceneStack_; }
        
    private:
        std::shared_ptr<GameCore::MainScenarioProgression> mainScenarioProgression_;
        const std::shared_ptr<Sub::IGameSceneStack> subSceneStack_;
    };
}
