#pragma once
#include <optional>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"

#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../../../../../Npc/Enemy/SpawnPoint/EnemySpawnPoint.h"
#include "../../../../../Story/Story_StageClear.h"
#include "../../GrassLand/ArrivalMovie/StageArrivalTourShot.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    /**
     * 古竜の巣 (DragonNestScene) のコンテキスト。持つものは砂漠 (DrySandSceneContext) から浮遊石を除いたもので、
     * 到着演出も同じ StageArrivalMovie を使う。嵐はシーンに置いた WeatherService が入場時から始める
     */
    class DragonNestSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;

        [[nodiscard]] std::shared_ptr<Asset::SoundFile> BGM() const { return bgm_.get(); }
        [[nodiscard]] GamePlay::Network::CustomNetworkRunner& NetworkRunner() const { return *networkRunner_.get(); }
        [[nodiscard]] std::weak_ptr<GamePlay::Network::CustomNetworkRunner> WeakNetworkRunner() const { return networkRunner_.get(); }
        /** enemySpawnPointsRoot_ の子孫のうち EnemySpawnPoint を持つもの。湧かせる種別は各地点が持つ */
        [[nodiscard]] std::vector<std::shared_ptr<Npc::Enemy::EnemySpawnPoint>> EnemySpawnPoints() const;

        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> ArrivalCamera() const { return arrivalCamera_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CinemachineCameraBrain>   CameraBrain  () const { return cameraBrain_  .get(); }
        [[nodiscard]] Asset::PrefabGameObjectFile& ArrivalPortalPrefab() const { return *arrivalPortalPrefab_.get(); }
        [[nodiscard]] bool HasArrivalPortalPrefab() const { return static_cast<bool>(arrivalPortalPrefab_); }
        [[nodiscard]] int ArrivalPortalOpenDelay_msecs () const { return arrivalPortalOpenDelay_msecs_;  }
        [[nodiscard]] int ArrivalPortalOpen_msecs      () const { return arrivalPortalOpen_msecs_;       }
        [[nodiscard]] int ArrivalWalk_msecs            () const { return arrivalWalk_msecs_;             }
        [[nodiscard]] int ArrivalPortalCloseDelay_msecs() const { return arrivalPortalCloseDelay_msecs_; }
        [[nodiscard]] int ArrivalPortalClose_msecs     () const { return arrivalPortalClose_msecs_;      }
        [[nodiscard]] int ArrivalHold_msecs            () const { return arrivalHold_msecs_;             }
        [[nodiscard]] float ArrivalPortalHeight   () const { return arrivalPortalHeight_;    }
        [[nodiscard]] float ArrivalWalkStartBehind() const { return arrivalWalkStartBehind_; }
        [[nodiscard]] float ArrivalWalkDistance   () const { return arrivalWalkDistance_;    }
        [[nodiscard]] const glm::vec3& ArrivalCameraStart() const { return arrivalCameraStart_; }
        [[nodiscard]] const glm::vec3& ArrivalCameraEnd  () const { return arrivalCameraEnd_;   }
        [[nodiscard]] float ArrivalLookAtHeight() const { return arrivalLookAtHeight_; }
        /** 島を見下ろす空撮の尺。0なら空撮せず、ポータルのショットから始める */
        [[nodiscard]] int ArrivalOverview_msecs       () const { return arrivalOverview_msecs_;        }
        /** 空撮の終点からポータルのショットの始点まで降りてくる尺 */
        [[nodiscard]] int ArrivalOverviewDescend_msecs() const { return arrivalOverviewDescend_msecs_; }
        /** 空撮のカメラの始点と終点 (ワールド座標) */
        [[nodiscard]] const glm::vec3& ArrivalOverviewCameraStart() const { return arrivalOverviewCameraStart_; }
        [[nodiscard]] const glm::vec3& ArrivalOverviewCameraEnd  () const { return arrivalOverviewCameraEnd_;   }
        /** 空撮で注視する島の中心 (ワールド座標) */
        [[nodiscard]] const glm::vec3& ArrivalOverviewLookAt     () const { return arrivalOverviewLookAt_;      }
        /** 初めて着いたときの空撮の1ショット目に出す島の名前と一言。名前が空なら字幕を出さない */
        [[nodiscard]] const std::string& ArrivalIslandTitle   () const { return arrivalIslandTitle_;    }
        [[nodiscard]] const std::string& ArrivalIslandSubtitle() const { return arrivalIslandSubtitle_; }
        /** 空撮のあとに巡る島の見どころ。最後の見どころからポータルへ降りるので、ポータルに近いものを最後に並べる */
        [[nodiscard]] const std::vector<GrassLand::StageArrivalTourShot>& ArrivalTourShots() const { return arrivalTourShots_; }
        /** 空撮の字幕 (StageArrivalCaption を持つプレハブ) */
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> ArrivalCaptionPrefab() const { return arrivalCaptionPrefab_.get(); }
        /** このステージのクリア条件。どちらかが -1 なら無し */
        [[nodiscard]] std::optional<Story::StageClearCondition> StageClear() const;

        /** 古竜を倒した後に空へ散る心臓。heartsRoot_ の子と、floatingRoot_ の子のうち名前が NestHeart で始まるもの */
        [[nodiscard]] std::vector<std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>> ScatterHearts() const;
        /** 心臓の山を映すカメラ (LookAt で山の上を見る) */
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> EndingCamera() const { return endingCamera_.get(); }
        /** 心臓の色ごとの光の尾。潮・宵は光のものを使う */
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> HeartTrail(const std::string& heartName) const;
        /** 心臓が抜け出す瞬間に山の上で出す閃光 */
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> HeartBurst() const { return heartBurst_.get(); }
        [[nodiscard]] const glm::vec3& HeartMoundCenter() const { return heartMoundCenter_; }
        [[nodiscard]] float EndingDelay_secs  () const { return endingDelay_secs_;   }
        [[nodiscard]] float HeartRise_secs    () const { return heartRise_secs_;     }
        [[nodiscard]] float HeartFly_secs     () const { return heartFly_secs_;      }
        [[nodiscard]] float HeartStagger_secs () const { return heartStagger_secs_;  }
        [[nodiscard]] float EndingHold_secs   () const { return endingHold_secs_;    }
        [[nodiscard]] float HeartRiseHeight   () const { return heartRiseHeight_;    }
        [[nodiscard]] float HeartFlyDistance  () const { return heartFlyDistance_;   }

    private:
        [[serialize(0)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(0)]] FIELD(GamePlay::Network::CustomNetworkRunner) networkRunner_;
        [[serialize(0)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) enemySpawnPointsRoot_;
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) arrivalCamera_;
        [[serialize(0)]] FIELD(CineMachine::CinemachineCameraBrain)   cameraBrain_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile)           arrivalPortalPrefab_;
        [[serialize(0)]] int       arrivalPortalOpenDelay_msecs_  = 300;
        [[serialize(0)]] int       arrivalPortalOpen_msecs_       = 800;
        [[serialize(0)]] int       arrivalWalk_msecs_             = 2200;
        [[serialize(0)]] int       arrivalPortalCloseDelay_msecs_ = 700;
        [[serialize(0)]] int       arrivalPortalClose_msecs_      = 600;
        [[serialize(0)]] int       arrivalHold_msecs_             = 500;
        [[serialize(0)]] float     arrivalPortalHeight_           = 10.0f;
        [[serialize(0)]] float     arrivalWalkStartBehind_        = 6.0f;
        [[serialize(0)]] float     arrivalWalkDistance_           = 50.0f;
        [[serialize(0)]] glm::vec3 arrivalCameraStart_            = glm::vec3(22.0f, 3.5f, 24.0f);
        [[serialize(0)]] glm::vec3 arrivalCameraEnd_              = glm::vec3(18.0f, 2.5f, 36.0f);
        [[serialize(0)]] float     arrivalLookAtHeight_           = 12.0f;
        // NOTE: tools.scene で設定できるよう EnemyKind / Story::StoryFlag を int で持つ
        [[serialize(0)]] int       clearEnemyKind_                = -1;
        [[serialize(0)]] int       clearStoryFlag_                = -1;
        [[serialize(0)]] int       arrivalOverview_msecs_        = 0;
        [[serialize(0)]] int       arrivalOverviewDescend_msecs_ = 3500;
        [[serialize(0)]] glm::vec3 arrivalOverviewCameraStart_   = glm::vec3(0.0f);
        [[serialize(0)]] glm::vec3 arrivalOverviewCameraEnd_     = glm::vec3(0.0f);
        [[serialize(0)]] glm::vec3 arrivalOverviewLookAt_        = glm::vec3(0.0f);
        [[serialize(2)]] std::string arrivalIslandTitle_;
        [[serialize(2)]] std::string arrivalIslandSubtitle_;
        [[serialize(2)]] std::vector<GrassLand::StageArrivalTourShot> arrivalTourShots_;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile) arrivalCaptionPrefab_;
        [[serialize(1)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) heartsRoot_;
        [[serialize(1)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) floatingRoot_;
        [[serialize(1)]] FIELD(CineMachine::CineMachineVirtualCamera) endingCamera_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) greenHeartTrail_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) lightHeartTrail_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) fireHeartTrail_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) heartBurst_;
        [[serialize(1)]] glm::vec3 heartMoundCenter_  = glm::vec3(750.0f, 124.0f, 720.0f);
        [[serialize(1)]] float     endingDelay_secs_  = 1.0f;
        [[serialize(1)]] float     heartRise_secs_    = 1.8f;
        [[serialize(1)]] float     heartFly_secs_     = 3.2f;
        [[serialize(1)]] float     heartStagger_secs_ = 2.4f;
        [[serialize(1)]] float     endingHold_secs_   = 3.0f;
        [[serialize(1)]] float     heartRiseHeight_   = 60.0f;
        [[serialize(1)]] float     heartFlyDistance_  = 3000.0f;

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
            archive(CEREAL_NVP(arrivalOverview_msecs_));
            archive(CEREAL_NVP(arrivalOverviewDescend_msecs_));
            archive(CEREAL_NVP(arrivalOverviewCameraStart_));
            archive(CEREAL_NVP(arrivalOverviewCameraEnd_));
            archive(CEREAL_NVP(arrivalOverviewLookAt_));
            archive(CEREAL_NVP(heartsRoot_));
            archive(CEREAL_NVP(floatingRoot_));
            archive(CEREAL_NVP(endingCamera_));
            archive(CEREAL_NVP(greenHeartTrail_));
            archive(CEREAL_NVP(lightHeartTrail_));
            archive(CEREAL_NVP(fireHeartTrail_));
            archive(CEREAL_NVP(heartBurst_));
            archive(CEREAL_NVP(heartMoundCenter_));
            archive(CEREAL_NVP(endingDelay_secs_));
            archive(CEREAL_NVP(heartRise_secs_));
            archive(CEREAL_NVP(heartFly_secs_));
            archive(CEREAL_NVP(heartStagger_secs_));
            archive(CEREAL_NVP(endingHold_secs_));
            archive(CEREAL_NVP(heartRiseHeight_));
            archive(CEREAL_NVP(heartFlyDistance_));
            archive(CEREAL_NVP(arrivalIslandTitle_));
            archive(CEREAL_NVP(arrivalIslandSubtitle_));
            archive(CEREAL_NVP(arrivalTourShots_));
            archive(CEREAL_NVP(arrivalCaptionPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<SceneContextBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bgm_));
            if (version >= 0) archive(CEREAL_NVP(networkRunner_));
            if (version >= 0) archive(CEREAL_NVP(enemySpawnPointsRoot_));
            if (version >= 0) archive(CEREAL_NVP(arrivalCamera_));
            if (version >= 0) archive(CEREAL_NVP(cameraBrain_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalPrefab_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalOpenDelay_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalOpen_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalWalk_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalCloseDelay_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalClose_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalHold_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalPortalHeight_));
            if (version >= 0) archive(CEREAL_NVP(arrivalWalkStartBehind_));
            if (version >= 0) archive(CEREAL_NVP(arrivalWalkDistance_));
            if (version >= 0) archive(CEREAL_NVP(arrivalCameraStart_));
            if (version >= 0) archive(CEREAL_NVP(arrivalCameraEnd_));
            if (version >= 0) archive(CEREAL_NVP(arrivalLookAtHeight_));
            if (version >= 0) archive(CEREAL_NVP(clearEnemyKind_));
            if (version >= 0) archive(CEREAL_NVP(clearStoryFlag_));
            if (version >= 0) archive(CEREAL_NVP(arrivalOverview_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalOverviewDescend_msecs_));
            if (version >= 0) archive(CEREAL_NVP(arrivalOverviewCameraStart_));
            if (version >= 0) archive(CEREAL_NVP(arrivalOverviewCameraEnd_));
            if (version >= 0) archive(CEREAL_NVP(arrivalOverviewLookAt_));
            if (version >= 1)
            {
                archive(CEREAL_NVP(heartsRoot_));
                archive(CEREAL_NVP(floatingRoot_));
                archive(CEREAL_NVP(endingCamera_));
                archive(CEREAL_NVP(greenHeartTrail_));
                archive(CEREAL_NVP(lightHeartTrail_));
                archive(CEREAL_NVP(fireHeartTrail_));
                archive(CEREAL_NVP(heartBurst_));
                archive(CEREAL_NVP(heartMoundCenter_));
                archive(CEREAL_NVP(endingDelay_secs_));
                archive(CEREAL_NVP(heartRise_secs_));
                archive(CEREAL_NVP(heartFly_secs_));
                archive(CEREAL_NVP(heartStagger_secs_));
                archive(CEREAL_NVP(endingHold_secs_));
                archive(CEREAL_NVP(heartRiseHeight_));
                archive(CEREAL_NVP(heartFlyDistance_));
            }
            if (version >= 2)
            {
                archive(CEREAL_NVP(arrivalIslandTitle_));
                archive(CEREAL_NVP(arrivalIslandSubtitle_));
                archive(CEREAL_NVP(arrivalTourShots_));
                archive(CEREAL_NVP(arrivalCaptionPrefab_));
            }
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::DragonNestSceneContext, 2);
#pragma endregion
