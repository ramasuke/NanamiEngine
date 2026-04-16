#pragma once
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

#include "../Type/SubSceneType.h"

namespace GameCore::Scene::Sub
{
    class SceneContextBase;
    class IGameScene;

    class SceneFactory final
    {
    public:
        using Creator = std::function<std::shared_ptr<IGameScene>()>;
        explicit SceneFactory(std::vector<std::weak_ptr<SceneContextBase>> contexts);

        std::shared_ptr<IGameScene> Create(const SceneType& type) const;

    private:
        void Register(const SceneType& type, Creator creator);
        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

        std::unordered_map<SceneType, Creator> creators_;
        std::vector<std::weak_ptr<SceneContextBase>> contexts_;
    };

    template<typename T>
    requires std::derived_from<T, SceneContextBase>
    std::shared_ptr<T> SceneFactory::CatchContext()
    {
        for (const auto& context : contexts_)
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