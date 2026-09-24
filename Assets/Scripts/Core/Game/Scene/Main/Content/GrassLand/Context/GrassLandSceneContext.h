#pragma once
#include <optional>

#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../../../../../Npc/Enemy/SpawnPoint/EnemySpawnPoint.h"
#include "../../../../../Story/Story_StageClear.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class GrassLandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>& BGM() const { return bgm_.get(); }
        [[nodiscard]] GamePlay::Network::CustomNetworkRunner& NetworkRunner() const { return *networkRunner_.get(); }
        [[nodiscard]] std::weak_ptr<GamePlay::Network::CustomNetworkRunner> WeakNetworkRunner() const { return networkRunner_.get(); }
        /** enemySpawnPointsRoot_ の子孫のうち EnemySpawnPoint を持つもの。湧かせる種別は各地点が持つ */
        [[nodiscard]] std::vector<std::shared_ptr<Npc::Enemy::EnemySpawnPoint>> EnemySpawnPoints() const;

        /**
         * 到着演出のあいだだけ優先度を上げるVirtualCamera
         * @note Follow/LookAtのtargetはPlayerSpawn Posのマーカーにしておくこと。演出はそこからのオフセットで動かす
         */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> ArrivalCamera() const { return arrivalCamera_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CinemachineCameraBrain>   CameraBrain  () const { return cameraBrain_  .get(); }
        /** 到着演出でスポーン地点に立てるポータル。プレハブのルートのスケールが開ききったときの大きさになる */
        [[nodiscard]] Asset::PrefabGameObjectFile& ArrivalPortalPrefab() const { return *arrivalPortalPrefab_.get(); }
        [[nodiscard]] bool HasArrivalPortalPrefab() const { return static_cast<bool>(arrivalPortalPrefab_); }
        /** ロード画面が明けてからポータルが開き始めるまで */
        [[nodiscard]] int ArrivalPortalOpenDelay_msecs () const { return arrivalPortalOpenDelay_msecs_;  }
        [[nodiscard]] int ArrivalPortalOpen_msecs      () const { return arrivalPortalOpen_msecs_;       }
        /** ポータルが開ききってから、プレイヤーが歩いて出てきて立ち止まるまで */
        [[nodiscard]] int ArrivalWalk_msecs            () const { return arrivalWalk_msecs_;             }
        /** 歩き始めてからポータルが閉じ始めるまで。プレイヤーが抜けきってから閉じるように取る */
        [[nodiscard]] int ArrivalPortalCloseDelay_msecs() const { return arrivalPortalCloseDelay_msecs_; }
        [[nodiscard]] int ArrivalPortalClose_msecs     () const { return arrivalPortalClose_msecs_;      }
        /** 立ち止まってから三人称カメラへ返すまで */
        [[nodiscard]] int ArrivalHold_msecs            () const { return arrivalHold_msecs_;             }
        /** ポータルの中心の地面からの高さ。膜の下側がプレイヤーの足元を隠せるよう、半径より少し低くする */
        [[nodiscard]] float ArrivalPortalHeight   () const { return arrivalPortalHeight_;    }
        /** 歩き出す位置をポータルの膜からどれだけ奥に取るか。体が膜から出ていると開く前に見えてしまう */
        [[nodiscard]] float ArrivalWalkStartBehind() const { return arrivalWalkStartBehind_; }
        /** 歩き出す位置から立ち止まる位置までの距離。尺で割った速さが歩行速度に近いと足が滑らない */
        [[nodiscard]] float ArrivalWalkDistance   () const { return arrivalWalkDistance_;    }
        /** カメラの始点。ポータルの足元から見た(横, カメラ直下の地面からの高さ, 歩く向き) */
        [[nodiscard]] const glm::vec3& ArrivalCameraStart() const { return arrivalCameraStart_; }
        /** カメラの終点。プレイヤーが横を通り過ぎたあと、斜め後ろから見送る位置に置くと三人称への戻りが短い */
        [[nodiscard]] const glm::vec3& ArrivalCameraEnd  () const { return arrivalCameraEnd_;   }
        /** 歩いているプレイヤーの、足元から注視点までの高さ */
        [[nodiscard]] float ArrivalLookAtHeight() const { return arrivalLookAtHeight_; }
        /** このステージのクリア条件。どちらかが -1 なら無し */
        [[nodiscard]] std::optional<Story::StageClearCondition> StageClear() const;

        /** 村の跡に落ちている緑の浮遊石(子にオーラ)。大顎を倒すと飛び去り、それ以降は出さない */
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> FloatingStone() const { return floatingStone_.get(); }
        /** 飛び去る石を LookAt で追うカメラ */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> FloatingStoneCamera() const { return floatingStoneCamera_.get(); }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> StoneLiftOffParticle() const { return stoneLiftOffParticle_.get(); }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> StoneFlightParticle () const { return stoneFlightParticle_ .get(); }

    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(2)]] FIELD(GamePlay::Network::CustomNetworkRunner) networkRunner_;
        [[serialize(6)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) enemySpawnPointsRoot_;
        [[serialize(8)]] FIELD(CineMachine::CineMachineVirtualCamera) arrivalCamera_;
        [[serialize(8)]] FIELD(CineMachine::CinemachineCameraBrain)   cameraBrain_;
        [[serialize(8)]] FIELD(Asset::PrefabGameObjectFile)           arrivalPortalPrefab_;
        [[serialize(9)]] int       arrivalPortalOpenDelay_msecs_  = 300;
        [[serialize(9)]] int       arrivalPortalOpen_msecs_       = 800;
        [[serialize(9)]] int       arrivalWalk_msecs_             = 2200;
        [[serialize(9)]] int       arrivalPortalCloseDelay_msecs_ = 700;
        [[serialize(9)]] int       arrivalPortalClose_msecs_      = 600;
        [[serialize(9)]] int       arrivalHold_msecs_             = 500;
        [[serialize(9)]] float     arrivalPortalHeight_           = 10.0f;
        [[serialize(9)]] float     arrivalWalkStartBehind_        = 6.0f;
        [[serialize(9)]] float     arrivalWalkDistance_           = 50.0f;
        [[serialize(9)]] glm::vec3 arrivalCameraStart_            = glm::vec3(22.0f, 3.5f, 24.0f);
        [[serialize(9)]] glm::vec3 arrivalCameraEnd_              = glm::vec3(18.0f, 2.5f, 36.0f);
        [[serialize(9)]] float     arrivalLookAtHeight_           = 12.0f;
        // NOTE: tools.scene で設定できるよう EnemyKind / Story::StoryFlag を int で持つ
        [[serialize(11)]] int      clearEnemyKind_                = -1;
        [[serialize(11)]] int      clearStoryFlag_                = -1;
        [[serialize(12)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) floatingStone_;
        [[serialize(12)]] FIELD(CineMachine::CineMachineVirtualCamera)         floatingStoneCamera_;
        [[serialize(12)]] FIELD(Asset::PrefabGameObjectFile)                   stoneLiftOffParticle_;
        [[serialize(12)]] FIELD(Asset::PrefabGameObjectFile)                   stoneFlightParticle_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(networkRunner_));
            archive(CEREAL_NVP(enemySpawnPointsRoot_));
            archive(CEREAL_NVP(arrivalCamera_));
            archive(CEREAL_NVP(cameraBrain_));
            archive(CEREAL_NVP(arrivalPortalPrefab_));
            archive(CEREAL_NVP(arrivalPortalOpenDelay_msecs_));
            archive(CEREAL_NVP(arrivalPortalOpen_msecs_));
            archive(CEREAL_NVP(arrivalWalk_msecs_));
            archive(CEREAL_NVP(arrivalPortalCloseDelay_msecs_));
            archive(CEREAL_NVP(arrivalPortalClose_msecs_));
            archive(CEREAL_NVP(arrivalHold_msecs_));
            archive(CEREAL_NVP(arrivalPortalHeight_));
            archive(CEREAL_NVP(arrivalWalkStartBehind_));
            archive(CEREAL_NVP(arrivalWalkDistance_));
            archive(CEREAL_NVP(arrivalCameraStart_));
            archive(CEREAL_NVP(arrivalCameraEnd_));
            archive(CEREAL_NVP(arrivalLookAtHeight_));
            archive(CEREAL_NVP(clearEnemyKind_));
            archive(CEREAL_NVP(clearStoryFlag_));
            archive(CEREAL_NVP(floatingStone_));
            archive(CEREAL_NVP(floatingStoneCamera_));
            archive(CEREAL_NVP(stoneLiftOffParticle_));
            archive(CEREAL_NVP(stoneFlightParticle_));
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
            // v7〜v9 は全地点で同じ種別を湧かせていた。今は地点ごとの EnemySpawnPoint が持つので読み捨てる
            [[serialize(7)]] Npc::Enemy::EnemyKind enemyKind_ = Npc::Enemy::EnemyKind::Hyena;
            if (version >= 7 && version <= 9) archive(CEREAL_NVP(enemyKind_));
            if (version >= 8)
            {
                archive(CEREAL_NVP(arrivalCamera_));
                archive(CEREAL_NVP(cameraBrain_));
                archive(CEREAL_NVP(arrivalPortalPrefab_));
            }
            if (version == 8)
            {
                // v8 は地中からせり上がる演出のショットを球面座標で持っていた。歩いて出てくる演出とは噛み合わないので読み捨てる
                [[serialize(8)]] int       arrivalShotDuring_msecs_  = 0;
                [[serialize(8)]] glm::vec3 arrivalShotStart_         = glm::vec3(0.0f);
                [[serialize(8)]] glm::vec3 arrivalShotEnd_           = glm::vec3(0.0f);
                [[serialize(8)]] glm::vec3 arrivalLookAtOffsetStart_ = glm::vec3(0.0f);
                [[serialize(8)]] glm::vec3 arrivalLookAtOffsetEnd_   = glm::vec3(0.0f);
                archive(CEREAL_NVP(arrivalShotDuring_msecs_));
                archive(CEREAL_NVP(arrivalShotStart_));
                archive(CEREAL_NVP(arrivalShotEnd_));
                archive(CEREAL_NVP(arrivalLookAtOffsetStart_));
                archive(CEREAL_NVP(arrivalLookAtOffsetEnd_));
            }
            if (version >= 9)
            {
                archive(CEREAL_NVP(arrivalPortalOpenDelay_msecs_));
                archive(CEREAL_NVP(arrivalPortalOpen_msecs_));
                archive(CEREAL_NVP(arrivalWalk_msecs_));
                archive(CEREAL_NVP(arrivalPortalCloseDelay_msecs_));
                archive(CEREAL_NVP(arrivalPortalClose_msecs_));
                archive(CEREAL_NVP(arrivalHold_msecs_));
                archive(CEREAL_NVP(arrivalPortalHeight_));
                archive(CEREAL_NVP(arrivalWalkStartBehind_));
                archive(CEREAL_NVP(arrivalWalkDistance_));
                archive(CEREAL_NVP(arrivalCameraStart_));
                archive(CEREAL_NVP(arrivalCameraEnd_));
                archive(CEREAL_NVP(arrivalLookAtHeight_));
            }
            if (version >= 11)
            {
                archive(CEREAL_NVP(clearEnemyKind_));
                archive(CEREAL_NVP(clearStoryFlag_));
            }
            if (version >= 12)
            {
                archive(CEREAL_NVP(floatingStone_));
                archive(CEREAL_NVP(floatingStoneCamera_));
                archive(CEREAL_NVP(stoneLiftOffParticle_));
                archive(CEREAL_NVP(stoneFlightParticle_));
            }
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::GrassLandSceneContext, 12);
#pragma endregion
