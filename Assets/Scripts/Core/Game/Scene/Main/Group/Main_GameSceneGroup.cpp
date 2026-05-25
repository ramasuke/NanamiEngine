#include "Main_GameSceneGroup.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../Content/MainIslandScene/MainIsLandScene.h"
#include "../Content/Title/TitleScene.h"

namespace GameCore::Scene::Main
{
    GameSceneGroup::GameSceneGroup(
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : sceneContexts_(std::move(sceneContexts))
    {
        AddScene(std::make_shared<TitleScene>(
            CatchContext<TitleSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(std::make_shared<FirstTouchDownMainIsLandScene>(
            CatchContext<FirstTouchDownMainIsLandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(std::make_shared<MainIslandScene>(
            CatchContext<MainIslandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));
    }

    void GameSceneGroup::Update()
    {
        ProcessRequests();
    }

    void GameSceneGroup::OnDrawGui()
    {
        for (const auto& scene : scenes_ | std::views::values)
        {
            scene->OnDrawGui();
        }
    }

    void GameSceneGroup::ProcessRequests()
    {
        for (const auto& changeRequest : changeRequests_)
        {
            Time::SkipNextFrame();
            Time::SkipNextFrame();
            
            if (const auto current = currentScene_.lock())
            {
                current->Dispose();
            }

            const auto& next = scenes_.at(changeRequest.type);
            next->Init();
            currentScene_ = next;
            currentScene_.lock()->Enter();
        }

        changeRequests_.clear();
    }
}