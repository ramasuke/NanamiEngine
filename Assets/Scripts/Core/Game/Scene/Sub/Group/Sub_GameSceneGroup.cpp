#include "Sub_GameSceneGroup.h"

#include <ranges>

#include "../Sub_IGameScene.h"
#include "../Factory/SubSceneFactory.h"

namespace GameCore::Scene::Sub
{
    GameSceneGroup::GameSceneGroup(std::vector<std::weak_ptr<SceneContextBase>> contexts)
        : factory_(std::make_unique<SceneFactory>(contexts))
    {
    }

    GameSceneGroup::~GameSceneGroup() = default;

    void GameSceneGroup::Push(const SceneType& type)
    {
        changeRequests_.emplace_back(ChangeRequestType::Push, type);
    }

    void GameSceneGroup::Pop(const SceneType& type)
    {
        changeRequests_.emplace_back(ChangeRequestType::Pop, type);
    }

    void GameSceneGroup::Update()
    {
        ProcessRequests();
    }

    void GameSceneGroup::OnDrawGui() const
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
            switch (changeRequest.type)
            {
            case ChangeRequestType::Push:
                {
                    const auto scene = factory_->Create(changeRequest.sceneType);
                    scene->Init();
                    scenes_[changeRequest.sceneType] = scene;
                    break;
                }
            case ChangeRequestType::Pop:
                {
                    auto it = scenes_.find(changeRequest.sceneType);
                    if (it == scenes_.end()) break;

                    it->second->Dispose();
                    scenes_.erase(it);
                    break;
                }
            }
        }

        changeRequests_.clear();
    }
}