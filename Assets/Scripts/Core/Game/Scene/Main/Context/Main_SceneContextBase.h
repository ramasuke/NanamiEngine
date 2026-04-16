#pragma once
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../../Engine/Module/Component/ComponentBase.h"

namespace GameCore::Scene
{
    class SceneContextBase : public Component::ComponentBase
    {
    public:
        virtual void Init();
        ~SceneContextBase() override = default;
        [[nodiscard]] std::shared_ptr<Asset::SceneFile> LoadSceneFile() const { return loadSceneFile_.get(); }
        [[nodiscard]] glm::vec3 PlayerSpawnPoint() const;

    private:
        FIELD(NanamiEngine::Module::Asset     ::SceneFile  ) loadSceneFile_;
        FIELD(NanamiEngine::Module::GameObject::IGameObject) playerSpawnPoint_;
#pragma region Serialization Function
public:
void BasedOnDrawgui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(CEREAL_NVP(loadSceneFile_));
    archive(CEREAL_NVP(playerSpawnPoint_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    if (version >= 1) archive(CEREAL_NVP(loadSceneFile_));
    if (version >= 1) archive(CEREAL_NVP(playerSpawnPoint_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::SceneContextBase, 1);
CEREAL_REGISTER_TYPE(GameCore::Scene::SceneContextBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Scene::SceneContextBase);
#pragma endregion