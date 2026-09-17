#pragma once
#include <memory>

#include "../../../../MainProgression/MainProgression.h"
#include "../../../../../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"

namespace GameCore::Scene::Sub
{
    class IGameSceneStack;
}

namespace GameCore::Scene::Main
{
    class GameSceneBaseContext final
    {
    public:
        GameSceneBaseContext(
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
            const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen);

        [[nodiscard]] Sub::IGameSceneStack& SubSceneStack() const { return *subSceneStack_; }
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return *loadingScreen_; }
        void ClearSubScenes() const;
        
    private:
        const std::shared_ptr<Sub::IGameSceneStack> subSceneStack_;
        const std::shared_ptr<GamePlay::Ui::LoadingScreenUi> loadingScreen_;
    };
}
