#include "Main_GameSceneGroup.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include <stdexcept>
#include "../Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../Content/GrassLand/GrassLandScene.h"
#include "../Content/MainIslandScene/MainIsLandScene.h"
#include "../Content/Title/TitleScene.h"

namespace GameCore::Scene::Main
{
    GameSceneGroup::GameSceneGroup(
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : sceneContexts_(std::move(sceneContexts))
    {
        AddScene(SceneType::Title, std::make_shared<TitleScene>(
            CatchContext<TitleSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::FirstTouchDownMainIsLand, std::make_shared<FirstTouchDownMainIsLandScene>(
            CatchContext<FirstTouchDownMainIsLandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::MainIsland, std::make_shared<MainIslandScene>(
            CatchContext<MainIslandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::GrassLand, std::make_shared<GrassLandScene>(
            CatchContext<GrassLandSceneContext>(),
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

    void GameSceneGroup::RequestChangeScene(const SceneType type)
    {
        assert(scenes_.contains(type) && "Scene not registered");

        changeRequests_.push_back(type);
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

            const auto& next = scenes_.at(changeRequest);
            next->Init();
            currentScene_ = next;
            currentScene_.lock()->Enter();
        }

        changeRequests_.clear();
    }

    void GameSceneGroup::AddScene(const SceneType type, std::shared_ptr<IGameScene> scene)
    {
        scenes_[type] = std::move(scene);
    }
}
