#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"

namespace GameCore::Scene::Sub
{
    class IGameSceneStack;
}

namespace GameCore::Scene::Sub
{
    class GameSceneGroup;
}

namespace GameCore::Scene::Main
{
    class GameSceneGroup;
}

namespace GameCore
{
    enum class GameProgresion;
}

namespace GameCore
{
    class Game final : public Component::ComponentBase,
                       public LifeCycleCallback::IAwakable,
                       public LifeCycleCallback::IUpdatable
    {
    public:
        Game();
        ~Game() override;
        void OnDrawGui() override;
        [[nodiscard]] static Game& Instance() { return *instance_; }
        [[nodiscard]] Scene::Main::GameSceneGroup& Scenes() const { return *sceneGroup_; }
        [[nodiscard]] Scene::Sub:: GameSceneGroup& SubScenes() const;
        [[nodiscard]] GamePlay::Ui::LoadingScreenUi& LoadingScreen() const { return *loadingScreen_; }

    private:
        void InitMainSceneGroup();
        void InitSubSceneGroup();
        /** @brief ロード画面のシーンを常駐させ、その LoadingScreenUi を掴む */
        void InitStageLoadingScene();
        /** @brief ゲームオーバー画面のシーンを常駐させる。中の GameOverPresenter が自分で死亡を見張る */
        void InitGameOverScene();
        void OnAwake () override;
        void OnUpdate() override;
        
        std::unique_ptr<Scene::Main::GameSceneGroup> sceneGroup_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) sceneContexts_;
        std::shared_ptr<Scene::Sub::GameSceneGroup> subSceneGroup_;
        [[serialize(2)]] FIELD(GameObject::IGameObject) subSceneContexts_;
        [[serialize(3)]] FIELD(Asset::SceneFile) stageLoadingSceneFile_;
        [[serialize(4)]] FIELD(Asset::SceneFile) gameOverSceneFile_;
        
        std::shared_ptr<GameProgresion> mainScenarioProgression_;
        std::weak_ptr<NanamiEngine::Scene::Scene> stageLoadingScene_;
        std::shared_ptr<GamePlay::Ui::LoadingScreenUi> loadingScreen_;
        static Game* instance_;
        
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    archive(CEREAL_NVP(sceneContexts_));
    archive(CEREAL_NVP(subSceneContexts_));
    archive(CEREAL_NVP(stageLoadingSceneFile_));
    archive(CEREAL_NVP(gameOverSceneFile_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    if (version >= 1) archive(CEREAL_NVP(sceneContexts_));
    if (version >= 2) archive(CEREAL_NVP(subSceneContexts_));
    if (version >= 3) archive(CEREAL_NVP(stageLoadingSceneFile_));
    if (version >= 4) archive(CEREAL_NVP(gameOverSceneFile_));
    instance_ = this;
}
#pragma endregion
};
}


ENGINE_REGISTER_COMPONENT(GameCore::Game, 4)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, GameCore::Game);
