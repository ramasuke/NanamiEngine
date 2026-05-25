#pragma once
#include "../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class MainIslandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }
        
    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(bgm_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 1) archive(CEREAL_NVP(bgm_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::MainIslandSceneContext, 1);
CEREAL_REGISTER_TYPE(GameCore::Scene::MainIslandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::MainIslandSceneContext);
#pragma endregion
