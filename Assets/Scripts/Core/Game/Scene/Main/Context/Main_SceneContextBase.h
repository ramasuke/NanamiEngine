#pragma once
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../PlayerAvatar/CameraGroup/AllPlayerCameraGroup.h"
#include "../../../PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"

namespace GameCore::Scene
{
    class SceneContextBase : public Component::ComponentBase
    {
    public:
        virtual void Init();
        ~SceneContextBase() override = default;
        [[nodiscard]] std::shared_ptr<Asset::SceneFile> LoadSceneFile() const { return loadSceneFile_.get(); }
        [[nodiscard]] glm::vec3 PlayerSpawnPoint() const;
        [[nodiscard]] PlayerAvatar::AllPlayerCameraGroup CameraGroup() const;
        [[nodiscard]] Asset::PlayerAvatarFactory&        PlayerAvatarFactory() const { return *playerAvatarFactory_.get(); }

    private:
        [[serialize(1)]] FIELD(NanamiEngine::Module::Asset     ::SceneFile  ) loadSceneFile_;
        [[serialize(1)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) playerSpawnPoint_;
        [[serialize(2)]] FIELD(PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
        [[serialize(3)]] FIELD(Asset::PlayerAvatarFactory)                        playerAvatarFactory_;
        
#pragma region Serialization Function
public:
void BasedOnDrawgui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(CEREAL_NVP(loadSceneFile_));
    archive(CEREAL_NVP(playerSpawnPoint_));
    archive(CEREAL_NVP(swordmanCameraGroup_));
    archive(CEREAL_NVP(playerAvatarFactory_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    if (version >= 1) archive(CEREAL_NVP(loadSceneFile_));
    if (version >= 1) archive(CEREAL_NVP(playerSpawnPoint_));
    if (version >= 2) archive(CEREAL_NVP(swordmanCameraGroup_));
    if (version >= 3) archive(CEREAL_NVP(playerAvatarFactory_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::SceneContextBase, 3);
CEREAL_REGISTER_TYPE(GameCore::Scene::SceneContextBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Scene::SceneContextBase);
#pragma endregion