#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <optional>
#include <cassert>

#include "RequestType.h"
#include "../../Main/Base/Main_GameSceneBase.h"
#include "../../Main/Context/Main_SceneContextBase.h"

namespace GameCore::Scene::Main
{
    class GameSceneGroup final
    {
    public:
        explicit GameSceneGroup(
            std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
            const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression,
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack);

        void OnUpdate();
        void OnDrawGui();

        template <typename T>
        requires std::derived_from<T, IGameScene>
        void RequestChangeScene();

    private:
        void ProcessRequests();

        template <typename T>
        requires std::derived_from<T, IGameScene>
        void AddScene(std::shared_ptr<T> scene);

        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

        std::unordered_map<std::type_index, std::shared_ptr<IGameScene>> scenes_;
        std::weak_ptr<IGameScene> currentScene_;
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts_;

        std::vector<SceneChangeRequest> changeRequests_;
    };

    template <typename T>
    requires std::derived_from<T, IGameScene>
    void GameSceneGroup::AddScene(std::shared_ptr<T> scene)
    {
        scenes_[std::type_index(typeid(T))] = std::move(scene);
    }

    template <typename T>
    requires std::derived_from<T, SceneContextBase>
    std::shared_ptr<T> GameSceneGroup::CatchContext()
    {
        for (const auto& context : sceneContexts_)
        {
            if (auto locked = context.lock())
            {
                if (auto typed = std::dynamic_pointer_cast<T>(locked))
                {
                    return typed;
                }
            }
        }
        return nullptr;
    }

    template <typename T>
    requires std::derived_from<T, IGameScene>
    void GameSceneGroup::RequestChangeScene()
    {
        assert(scenes_.contains(typeid(T)) && "Scene not registered");

        changeRequests_.push_back(SceneChangeRequest(std::type_index(typeid(T))));
    }
}