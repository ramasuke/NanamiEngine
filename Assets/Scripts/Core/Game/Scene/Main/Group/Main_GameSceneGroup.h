#pragma once
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

#include "../../Main/Base/Main_GameSceneBase.h"
#include "../../Main/Context/Main_SceneContextBase.h"
#include "../Type/MainSceneType.h"

namespace GameCore::Scene::Main
{
    class GameSceneGroup final
    {
    public:
        GameSceneGroup(
            std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
            const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen);

        void Update();
        void OnDrawGui();
        void RequestChangeScene(SceneType type);
        /** @brief 最後に切り替えが成功したシーン。まだ一度も切り替えていなければ空 */
        [[nodiscard]] std::optional<SceneType> CurrentSceneType() const { return currentSceneType_; }
        /** @brief 次の Update で処理される切り替え要求が残っているか */
        [[nodiscard]] bool HasPendingChange() const { return !changeRequests_.empty(); }
        
        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

        template<typename T>
        requires std::derived_from<T, IGameScene>
        std::shared_ptr<T> Catch(SceneType type) const;

    private:
        void ProcessRequests();
        void AddScene(SceneType type, std::shared_ptr<IGameScene> scene);

        std::unordered_map<SceneType, std::shared_ptr<IGameScene>> scenes_;
        std::weak_ptr<IGameScene> currentScene_;
        std::optional<SceneType> currentSceneType_;
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts_;

        std::vector<SceneType> changeRequests_;
    };

    template <typename T>
    requires std::derived_from<T, IGameScene>
    std::shared_ptr<T> GameSceneGroup::Catch(const SceneType type) const
    {
        const auto it = scenes_.find(type);
        if (it == scenes_.end())
            return nullptr;

        return std::dynamic_pointer_cast<T>(it->second);
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
}
