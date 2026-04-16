#pragma once
#include "../../Scene/Scene.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"

constexpr auto SCENE_FILE_EXTENSION_LABEL = ".scene";

namespace NanamiEngine::Module::Asset
{
    class SceneFile final : public AssetBase
    {
    public:
        explicit SceneFile(std::string contentPath = "");
        [[nodiscard]] const Guid& GetGuid() const override  { return guid_; }
        std::shared_ptr<Scene::Scene> LoadScene() const;
        [[nodiscard]] std::string GetContentPath() const override;
        
    private:
        void OnDoubleClick() override;
        
        [[serialize(0)]] std::string contentPath_;
        [[serialize(0)]] Guid guid_;
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<AssetBase>(this));
    archive(CEREAL_NVP(contentPath_));
    archive(CEREAL_NVP(guid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<AssetBase>(this));
    if (version >= 0) archive(CEREAL_NVP(contentPath_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
}
#pragma endregion
};
}


#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SceneFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SceneFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::SceneFile);
#pragma endregion
REGISTER_ASSET(SceneFile, SCENE_FILE_EXTENSION_LABEL)
REGISTER_CREATABLE_ASSET_EXTENSION("Scene", SCENE_FILE_EXTENSION_LABEL)