#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"

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
    enum class MainScenarioProgression;
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

    private:
        void OnAwake () override;
        void OnUpdate() override;
        
        std::unique_ptr<Scene::Main::GameSceneGroup> sceneGroup_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) sceneContexts_;
        std::shared_ptr<Scene::Sub::GameSceneGroup> subSceneGroup_;
        [[serialize(2)]] FIELD(GameObject::IGameObject) subSceneContexts_;
        
        std::shared_ptr<MainScenarioProgression> mainScenarioProgression_;
        static Game* instance_;
        
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    archive(CEREAL_NVP(sceneContexts_));
    archive(CEREAL_NVP(subSceneContexts_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    if (version >= 1) archive(CEREAL_NVP(sceneContexts_));
    if (version >= 2) archive(CEREAL_NVP(subSceneContexts_));
    instance_ = this;
}
#pragma endregion
};
}


#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Game, 2);
CEREAL_REGISTER_TYPE(GameCore::Game);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Game);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, GameCore::Game);
#pragma endregion
