#pragma once
#include <unordered_map>
#include <memory>
#include <vector>

#include "../../Main/Base/Main_GameSceneBase.h"
#include "../../Main/Context/Main_SceneContextBase.h"
#include "../Type/MainSceneType.h"

namespace GameCore::Scene::Main
{
    class GameSceneGroup final
    {
    public:
        explicit GameSceneGroup(
            std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack);

        void Update();
        void OnDrawGui();
        void RequestChangeScene(SceneType type);
        
        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

    private:
        void ProcessRequests();
        void AddScene(SceneType type, std::shared_ptr<IGameScene> scene);

        std::unordered_map<SceneType, std::shared_ptr<IGameScene>> scenes_;
        std::weak_ptr<IGameScene> currentScene_;
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts_;

        std::vector<SceneType> changeRequests_;
    };

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
}
