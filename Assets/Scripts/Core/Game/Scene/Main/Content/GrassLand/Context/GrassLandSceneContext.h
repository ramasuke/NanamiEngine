#pragma once
#include "../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../../../../../Npc/Enemy/Type/EnemyKind.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class GrassLandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }
        [[nodiscard]] GamePlay::Network::CustomNetworkRunner& NetworkRunner() const { return *networkRunner_.get(); }
        [[nodiscard]] Npc::Enemy::EnemyKind EnemyKind() const { return enemyKind_; }
        [[nodiscard]] std::vector<std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>> EnemySpawnPoints() const;

        /** 到着演出のあいだだけ優先度を上げるVirtualCamera */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> ArrivalCamera() const { return arrivalCamera_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CinemachineCameraBrain>   CameraBrain  () const { return cameraBrain_  .get(); }
        /** 到着演出でスポーン地点に出す円形ポータル */
        [[nodiscard]] Asset::PrefabGameObjectFile& ArrivalPortalPrefab() const { return *arrivalPortalPrefab_.get(); }
        [[nodiscard]] bool HasArrivalPortalPrefab() const { return static_cast<bool>(arrivalPortalPrefab_); }
        [[nodiscard]] int ArrivalShotDuring_msecs() const { return arrivalShotDuring_msecs_; }
        /** 到着演出の始点。(スポーン地点まわりの水平角[deg], 仰俯角[deg], 距離) */
        [[nodiscard]] const glm::vec3& ArrivalShotStart() const { return arrivalShotStart_; }
        /** 到着演出の終点。水平角をプレイヤー背面へ寄せるほど三人称への戻りが短くなる */
        [[nodiscard]] const glm::vec3& ArrivalShotEnd  () const { return arrivalShotEnd_;   }
        [[nodiscard]] const glm::vec3& ArrivalLookAtOffsetStart() const { return arrivalLookAtOffsetStart_; }
        [[nodiscard]] const glm::vec3& ArrivalLookAtOffsetEnd  () const { return arrivalLookAtOffsetEnd_;   }

    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(2)]] FIELD(GamePlay::Network::CustomNetworkRunner) networkRunner_;
        [[serialize(6)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) enemySpawnPointsRoot_;
        [[serialize(7)]] Npc::Enemy::EnemyKind enemyKind_ = Npc::Enemy::EnemyKind::Hyena;
        [[serialize(8)]] FIELD(CineMachine::CineMachineVirtualCamera) arrivalCamera_;
        [[serialize(8)]] FIELD(CineMachine::CinemachineCameraBrain)   cameraBrain_;
        [[serialize(8)]] FIELD(Asset::PrefabGameObjectFile)           arrivalPortalPrefab_;
        [[serialize(8)]] int       arrivalShotDuring_msecs_  = 1800;
        [[serialize(8)]] glm::vec3 arrivalShotStart_         = glm::vec3(160.0f,  4.0f, 26.0f);
        [[serialize(8)]] glm::vec3 arrivalShotEnd_           = glm::vec3( 10.0f, 18.0f, 60.0f);
        [[serialize(8)]] glm::vec3 arrivalLookAtOffsetStart_ = glm::vec3(0.0f,  4.0f, 0.0f);
        [[serialize(8)]] glm::vec3 arrivalLookAtOffsetEnd_   = glm::vec3(0.0f, 22.0f, 0.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(networkRunner_));
            archive(CEREAL_NVP(enemySpawnPointsRoot_));
            archive(CEREAL_NVP(enemyKind_));
            archive(CEREAL_NVP(arrivalCamera_));
            archive(CEREAL_NVP(cameraBrain_));
            archive(CEREAL_NVP(arrivalPortalPrefab_));
            archive(CEREAL_NVP(arrivalShotDuring_msecs_));
            archive(CEREAL_NVP(arrivalShotStart_));
            archive(CEREAL_NVP(arrivalShotEnd_));
            archive(CEREAL_NVP(arrivalLookAtOffsetStart_));
            archive(CEREAL_NVP(arrivalLookAtOffsetEnd_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<SceneContextBase>(this));
            if (version >= 1) archive(CEREAL_NVP(bgm_));
            if (version >= 2) archive(CEREAL_NVP(networkRunner_));
            // v5〜v6 は湧かせる敵をプレハブ直指定で持っていた。今は EnemyFactory 側にあるので読み捨てる
            [[serialize(5)]] FIELD(Asset::PrefabGameObjectFile) enemyPrefab_;
            if (version >= 5 && version <= 6) archive(CEREAL_NVP(enemyPrefab_));
            if (version >= 6) archive(CEREAL_NVP(enemySpawnPointsRoot_));
            if (version >= 7) archive(CEREAL_NVP(enemyKind_));
            if (version >= 8)
            {
                archive(CEREAL_NVP(arrivalCamera_));
                archive(CEREAL_NVP(cameraBrain_));
                archive(CEREAL_NVP(arrivalPortalPrefab_));
                archive(CEREAL_NVP(arrivalShotDuring_msecs_));
                archive(CEREAL_NVP(arrivalShotStart_));
                archive(CEREAL_NVP(arrivalShotEnd_));
                archive(CEREAL_NVP(arrivalLookAtOffsetStart_));
                archive(CEREAL_NVP(arrivalLookAtOffsetEnd_));
            }
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::GrassLandSceneContext, 8);
CEREAL_REGISTER_TYPE(GameCore::Scene::GrassLandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::GrassLandSceneContext);
#pragma endregion
