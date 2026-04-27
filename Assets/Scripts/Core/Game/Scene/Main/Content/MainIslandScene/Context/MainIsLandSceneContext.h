#pragma once
#include "../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class MainIslandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

         const std::shared_ptr<Asset::PrefabGameObjectFile>& SummonPlayerAvatarPrefab() const { return summonPlayerAvatarPrefab_.get(); } 
        
    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) summonPlayerAvatarPrefab_;
        
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(summonPlayerAvatarPrefab_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 0) archive(CEREAL_NVP(summonPlayerAvatarPrefab_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::MainIslandSceneContext, 0);
CEREAL_REGISTER_TYPE(GameCore::Scene::MainIslandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::MainIslandSceneContext);
#pragma endregion
