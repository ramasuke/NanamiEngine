#pragma once
#include "../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class GrassLandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }
        [[nodiscard]] GamePlay::Network::CustomNetworkRunner& NetworkRunner() const { return *networkRunner_.get(); }
        
    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(2)]] FIELD(GamePlay::Network::CustomNetworkRunner) networkRunner_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(networkRunner_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<SceneContextBase>(this));
            if (version >= 1) archive(CEREAL_NVP(bgm_));
            if (version >= 2) archive(CEREAL_NVP(networkRunner_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::GrassLandSceneContext, 2);
CEREAL_REGISTER_TYPE(GameCore::Scene::GrassLandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::GrassLandSceneContext);
#pragma endregion
