#pragma once
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

#include "../../Main/Base/Main_GameSceneBase.h"
#include "../../Main/Context/Main_SceneContextBase.h"
#include "../Transition/Main_SceneTransitionOptions.h"
#include "../Type/MainSceneType.h"

namespace GameCore::Scene::Main
{
    /**
     * @brief メインシーンの切り替えを受け持つ。
     *
     * 切り替えは必ずロード画面を挟む: 覆い切るのを待つ → 旧シーンを Dispose・新シーンを Init
     * → 新シーンが IsEntered になるまで待つ → ロード画面を明ける。
     * 途中で来た要求は最後の 1 件だけ残し、覆い切っていればそのまま切り替え直す
     */
    class GameSceneGroup final
    {
    public:
        GameSceneGroup(
            std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
            const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
            const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen);

        void Update();
        void OnDrawGui();
        void RequestChangeScene(SceneType type, SceneTransitionOptions options = {});
        /** @brief 最後に切り替えたシーン。まだ一度も切り替えていなければ空 */
        [[nodiscard]] std::optional<SceneType> CurrentSceneType() const { return currentSceneType_; }
        /** @brief 切り替えの要求が残っているか、切り替えの途中(ロード画面が覆っている間)か */
        [[nodiscard]] bool HasPendingChange() const { return pendingRequest_.has_value() || phase_ != Phase::Idle; }
        
        template<typename T>
        requires std::derived_from<T, SceneContextBase>
        std::shared_ptr<T> CatchContext();

        template<typename T>
        requires std::derived_from<T, IGameScene>
        std::shared_ptr<T> Catch(SceneType type) const;

    private:
        struct ChangeRequest
        {
            SceneType              type;
            SceneTransitionOptions options;
        };

        enum class Phase : std::uint8_t
        {
            Idle,
            /** ロード画面が覆い切るのを待っている */
            Covering,
            /** 新シーンの入場(読み込み)を待っている */
            Entering,
        };

        /** @brief 残っている要求をロード画面に載せ、覆い切るのを待ち始める */
        void BeginCovering();
        void SwitchScene(const ChangeRequest& request);
        void AddScene(SceneType type, std::shared_ptr<IGameScene> scene);

        std::unordered_map<SceneType, std::shared_ptr<IGameScene>> scenes_;
        std::weak_ptr<IGameScene> currentScene_;
        std::optional<SceneType> currentSceneType_;
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts_;

        std::shared_ptr<GamePlay::Ui::LoadingScreenUi> loadingScreen_;
        std::optional<ChangeRequest> pendingRequest_;
        std::optional<ChangeRequest> coveringRequest_;
        Phase phase_ = Phase::Idle;
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
