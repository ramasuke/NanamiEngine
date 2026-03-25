#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>

#include "../../Base/GameSceneBase.h"
#include "../../Context/SceneContextBase.h"

namespace GameCore::Scene
{
    class GameMainSceneGroup final
    {
    public:
        explicit GameMainSceneGroup(
            std::vector<std::weak_ptr<SceneContextBase>> sceneContexts
            , const std::shared_ptr<MainScenarioProgression>& mainScenarioProgression);

        void OnUpdate();
        template <typename T>
        requires std::derived_from<T, IGameScene>
        void RequestChangeScene();
        void OnDrawGui();

    private:
        /** @brief requestされたSceneにcurrentSceneを変更 */
        void TryProcessChangeSceneRequest();
        template <typename T>
        requires std::derived_from<T, IGameScene>
        void AddScene(std::shared_ptr<T> scene);
        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

        std::unordered_map<std::type_index, std::shared_ptr<IGameScene>> scenes_;
        std::weak_ptr<IGameScene> currentScene_;
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts_;
        std::optional<std::type_index> sceneChangeRequest_;
    };

    template <typename T>
    requires std::derived_from<T, IGameScene>
    void GameMainSceneGroup::AddScene(std::shared_ptr<T> scene)
    {
        const std::type_index index(typeid(T));
        scenes_[index] = std::move(scene);
    }

    template <typename T>
    requires std::derived_from<T, SceneContextBase>
    std::shared_ptr<T> GameMainSceneGroup::CatchContext()
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
    void GameMainSceneGroup::RequestChangeScene()
    {
        assert(scenes_.contains(typeid(T)) && "Scene not found in scenes_. Did you forget to AddScene()?");

        sceneChangeRequest_ = std::type_index(typeid(T));
    }
}
