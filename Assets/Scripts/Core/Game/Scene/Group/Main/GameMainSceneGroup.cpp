#include "GameMainSceneGroup.h"

#include <utility>

#include "../../Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../Title/TitleScene.h"

GameCore::Scene::GameMainSceneGroup::GameMainSceneGroup(std::vector<std::weak_ptr<SceneContextBase>> sceneContexts
                                                        , const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression)
    : sceneContexts_(std::move(sceneContexts))
{
    AddScene(std::make_shared<TitleScene>(CatchContext<TitleSceneContext>(), GameSceneBaseContext(mainScenarioProgression)));
    AddScene(std::make_shared<FirstTouchDownMainIsLandScene>(CatchContext<FirstTouchDownMainIsLandSceneContext>(), GameSceneBaseContext(mainScenarioProgression)));
}

void GameCore::Scene::GameMainSceneGroup::OnUpdate()
{
    TryProcessChangeSceneRequest();
}

void GameCore::Scene::GameMainSceneGroup::OnDrawGui()
{
    for (const auto& scene : scenes_ | std::views::values)
    {
        scene->OnDrawGui();
    }
}

void GameCore::Scene::GameMainSceneGroup::TryProcessChangeSceneRequest()
{
    if (!sceneChangeRequest_.has_value())
        return;
    
    if (const auto currentScene = currentScene_.lock())
    {
        currentScene->OnExit ();
        currentScene->Dispose();
    }
    scenes_[*sceneChangeRequest_]->Init();
    currentScene_ = scenes_.at(*sceneChangeRequest_);
    currentScene_.lock()->OnEnter();
    sceneChangeRequest_.reset();
}
