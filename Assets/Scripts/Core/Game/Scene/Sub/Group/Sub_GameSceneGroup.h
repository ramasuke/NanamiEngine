#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

#include "ChangeRequest.h"
#include "Sub_IGameSceneGroup.h"

namespace GameCore::Scene::Sub
{
    class SceneFactory;
    class SceneContextBase;
    class IGameScene;
    enum class SceneType;

    class GameSceneGroup final : public IGameSceneStack
    {
    public:
        explicit GameSceneGroup(std::vector<std::weak_ptr<SceneContextBase>> contexts);
        ~GameSceneGroup() override;

        void Push(const SceneType& type) override;
        void Pop(const SceneType& type) override;

        void Update();
        void OnDrawGui() const;

        template <class T>
        requires std::is_base_of_v<IGameScene, T>
        std::shared_ptr<T> Catch(const SceneType& type) const;

    private:
        void ProcessRequests();

        std::unordered_map<SceneType, std::shared_ptr<IGameScene>> scenes_;
        std::unique_ptr<SceneFactory> factory_;

        std::vector<ChangeRequestOption> changeRequests_;
    };

    template <class T>
    requires std::is_base_of_v<IGameScene, T>
    std::shared_ptr<T> GameSceneGroup::Catch(const SceneType& type) const
    {
        const auto it = scenes_.find(type);
        if (it == scenes_.end())
            return nullptr;

        return std::dynamic_pointer_cast<T>(it->second);
    }
}