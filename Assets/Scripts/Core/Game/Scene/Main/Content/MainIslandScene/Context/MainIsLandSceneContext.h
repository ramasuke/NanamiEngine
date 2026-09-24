#pragma once
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class MainIslandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }

        /** 島の底に戻った緑の浮遊石(子にオーラ)。シーン上の位置がはまった位置 */
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> GreenStone() const { return greenStone_.get(); }
        /** 戻ってくる石を LookAt で追うカメラ */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> StoneCamera() const { return stoneCamera_.get(); }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> StoneDockParticle  () const { return stoneDockParticle_  .get(); }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> StoneFlightParticle() const { return stoneFlightParticle_.get(); }

        /** 草原の後に戻ってくる噴水の島。シーン上の位置が戻った位置 */
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> FountainIsland() const { return fountainIsland_.get(); }
        /** 噴水の島へ上る階段。子が1段ずつの足場 */
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> FountainStairs() const { return fountainStairs_.get(); }
        /** 戻ってくる島を LookAt で追うカメラ */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> FountainCamera() const { return fountainCamera_.get(); }
        /** カメラが見る島の子 */
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> FountainFocus() const { return fountainFocus_.get(); }
        
    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(2)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) greenStone_;
        [[serialize(2)]] FIELD(CineMachine::CineMachineVirtualCamera)         stoneCamera_;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile)                   stoneDockParticle_;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile)                   stoneFlightParticle_;
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) fountainIsland_;
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) fountainStairs_;
        [[serialize(3)]] FIELD(CineMachine::CineMachineVirtualCamera)         fountainCamera_;
        [[serialize(3)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) fountainFocus_;
        
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(bgm_));
    archive(CEREAL_NVP(greenStone_));
    archive(CEREAL_NVP(stoneCamera_));
    archive(CEREAL_NVP(stoneDockParticle_));
    archive(CEREAL_NVP(stoneFlightParticle_));
    archive(CEREAL_NVP(fountainIsland_));
    archive(CEREAL_NVP(fountainStairs_));
    archive(CEREAL_NVP(fountainCamera_));
    archive(CEREAL_NVP(fountainFocus_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 1) archive(CEREAL_NVP(bgm_));
    if (version >= 2)
    {
        archive(CEREAL_NVP(greenStone_));
        archive(CEREAL_NVP(stoneCamera_));
        archive(CEREAL_NVP(stoneDockParticle_));
        archive(CEREAL_NVP(stoneFlightParticle_));
    }
    if (version >= 3)
    {
        archive(CEREAL_NVP(fountainIsland_));
        archive(CEREAL_NVP(fountainStairs_));
        archive(CEREAL_NVP(fountainCamera_));
        archive(CEREAL_NVP(fountainFocus_));
    }
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::MainIslandSceneContext, 3);
#pragma endregion
