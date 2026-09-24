#pragma once
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../Context/Main_SceneContextBase.h"
#include "../../../../../../../GamePlay/Prop/FloatingStone/Prop_FloatingStone.h"
#include "../../../../../../../GamePlay/Prop/ReturningIsland/Prop_ReturningIsland.h"

namespace GameCore::Scene
{
    class MainIslandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }

        /** 島の底に戻った緑の浮遊石。シーン上の位置がはまった位置 */
        [[nodiscard]] std::shared_ptr<GamePlay::Prop::FloatingStone> GreenStone() const { return greenStone_.get(); }
        /** 草原の後に戻ってくる噴水の島。シーン上の位置が戻った位置 */
        [[nodiscard]] std::shared_ptr<GamePlay::Prop::ReturningIsland> FountainIsland() const { return fountainIsland_.get(); }
        
    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(5)]] FIELD(GamePlay::Prop::FloatingStone)   greenStone_;
        [[serialize(5)]] FIELD(GamePlay::Prop::ReturningIsland) fountainIsland_;
        
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(bgm_));
    archive(CEREAL_NVP(greenStone_));
    archive(CEREAL_NVP(fountainIsland_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 1) archive(CEREAL_NVP(bgm_));
    // v2〜v4 は石・島・カメラ・パーティクル・尺を別々に持っていた。今は石の FloatingStone と島の ReturningIsland が持つので読み捨てる
    if (version >= 2 && version <= 4)
    {
        [[serialize(2)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) oldStone;
        [[serialize(2)]] FIELD(CineMachine::CineMachineVirtualCamera)         oldStoneCamera;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile)                   oldDockParticle;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile)                   oldFlightParticle;
        archive(cereal::make_nvp("greenStone_",          oldStone));
        archive(cereal::make_nvp("stoneCamera_",         oldStoneCamera));
        archive(cereal::make_nvp("stoneDockParticle_",   oldDockParticle));
        archive(cereal::make_nvp("stoneFlightParticle_", oldFlightParticle));
    }
    if (version >= 3 && version <= 4)
    {
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) oldIsland;
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) oldStairs;
        [[serialize(3)]] FIELD(CineMachine::CineMachineVirtualCamera)         oldIslandCamera;
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) oldFocus;
        archive(cereal::make_nvp("fountainIsland_", oldIsland));
        archive(cereal::make_nvp("fountainStairs_", oldStairs));
        archive(cereal::make_nvp("fountainCamera_", oldIslandCamera));
        archive(cereal::make_nvp("fountainFocus_",  oldFocus));
    }
    if (version == 4)
    {
        [[serialize(4)]] GamePlay::Prop::ReturnShot       oldStoneShot;
        [[serialize(4)]] GamePlay::Prop::IslandReturnShot oldIslandShot;
        archive(cereal::make_nvp("stoneReturnShot_",    oldStoneShot));
        archive(cereal::make_nvp("fountainReturnShot_", oldIslandShot));
    }
    if (version >= 5)
    {
        archive(CEREAL_NVP(greenStone_));
        archive(CEREAL_NVP(fountainIsland_));
    }
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::MainIslandSceneContext, 5);
#pragma endregion
