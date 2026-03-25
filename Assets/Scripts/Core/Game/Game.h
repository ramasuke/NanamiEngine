#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"

namespace GameCore
{
    enum class MainScenarioProgression;
}

namespace GameCore::Scene
{
    class GameMainSceneGroup;
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
        [[nodiscard]] Scene::GameMainSceneGroup& MainScene() const { return *sceneGroup_; }
        
    private:
        void OnAwake () override;
        void OnUpdate() override;
        
        std::unique_ptr<Scene::GameMainSceneGroup> sceneGroup_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) sceneContexts_;
        
        std::shared_ptr<MainScenarioProgression> mainScenarioProgression_;
        static Game* instance_;
        
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    archive(CEREAL_NVP(sceneContexts_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(cereal::base_class<NanamiEngine::Module::LifeCycleCallback::IAwakable>(this));
    if (version >= 1) archive(CEREAL_NVP(sceneContexts_));
    instance_ = this;
}
#pragma endregion
};
}


#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Game, 1);
CEREAL_REGISTER_TYPE(GameCore::Game);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Game);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, GameCore::Game);
#pragma endregion
