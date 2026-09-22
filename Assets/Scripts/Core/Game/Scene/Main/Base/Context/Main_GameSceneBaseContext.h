#pragma once
#include <functional>
#include <memory>

#include "../../../../MainProgression/MainProgression.h"
#include "../../../../../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"
#include "../../Transition/Main_SceneTransitionOptions.h"
#include "../../Type/MainSceneType.h"

namespace GameCore::Scene::Sub
{
    class IGameSceneStack;
}

namespace GameCore::Scene::Main
{
    class GameSceneBaseContext final
    {
    public:
        using RequestChangeSceneFunc = std::function<void(SceneType, SceneTransitionOptions)>;

        GameSceneBaseContext(
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
            const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen,
            RequestChangeSceneFunc requestChangeScene);

        [[nodiscard]] Sub::IGameSceneStack& SubSceneStack() const { return *subSceneStack_; }
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return *loadingScreen_; }
        void ClearSubScenes() const;
        /** @brief GameSceneGroup へ遷移を頼む。シーン自身が別のシーンへ逃がすときに使う */
        void RequestChangeScene(const SceneType type, const SceneTransitionOptions options = {}) const { requestChangeScene_(type, options); }
        
    private:
        const std::shared_ptr<Sub::IGameSceneStack> subSceneStack_;
        const std::shared_ptr<GamePlay::Ui::LoadingScreenUi> loadingScreen_;
        RequestChangeSceneFunc requestChangeScene_;
    };
}
