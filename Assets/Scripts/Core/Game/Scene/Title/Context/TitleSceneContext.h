#pragma once
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../Context/SceneContextBase.h"

namespace GameCore::Scene
{
    class TitleSceneContext final : public SceneContextBase
    {
    public:
        std::shared_ptr<Asset::SceneFile> SceneFile() { return titleSceneFile_.get(); }
        
    private:
        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::SceneFile) titleSceneFile_;
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("titleSceneFile_", titleSceneFile_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(titleSceneFile_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 0) archive(CEREAL_NVP(titleSceneFile_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::TitleSceneContext, 0);
CEREAL_REGISTER_TYPE(GameCore::Scene::TitleSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::TitleSceneContext);
#pragma endregion
