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
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack);

        [[nodiscard]] Sub::IGameSceneStack& SubSceneStack() const { return *subSceneStack_; }
        
    private:
        const std::shared_ptr<Sub::IGameSceneStack> subSceneStack_;
    };
}
