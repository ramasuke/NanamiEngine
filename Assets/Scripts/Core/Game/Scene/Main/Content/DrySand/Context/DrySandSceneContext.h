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
#include "../../../../../../../GamePlay/Prop/FloatingStone/Prop_FloatingStone.h"
#include "../../GrassLand/ArrivalMovie/StageArrivalTourShot.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    /**
     * 砂漠 (DrySandScene) のコンテキスト。持つものは草原 (GrassLandSceneContext) と同じで、
     * 到着演出も同じ StageArrivalMovie を使う。浮遊石は神殿前の広場に落ちている光の浮遊石
     */
    class DrySandSceneContext final : public SceneContextBase
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

        /** 神殿前の広場に落ちている光の浮遊石。骸竜を倒すと飛び去り、それ以降は出さない */
        [[nodiscard]] std::shared_ptr<GamePlay::Prop::FloatingStone> FloatingStone() const { return floatingStone_.get(); }

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
        [[serialize(2)]] FIELD(GamePlay::Prop::FloatingStone) floatingStone_;
        [[serialize(3)]] int       arrivalOverview_msecs_        = 0;
        [[serialize(3)]] int       arrivalOverviewDescend_msecs_ = 3500;
        [[serialize(3)]] glm::vec3 arrivalOverviewCameraStart_   = glm::vec3(0.0f);
        [[serialize(3)]] glm::vec3 arrivalOverviewCameraEnd_     = glm::vec3(0.0f);
        [[serialize(3)]] glm::vec3 arrivalOverviewLookAt_        = glm::vec3(0.0f);
        [[serialize(4)]] std::string arrivalIslandTitle_;
        [[serialize(4)]] std::string arrivalIslandSubtitle_;
        [[serialize(4)]] std::vector<GrassLand::StageArrivalTourShot> arrivalTourShots_;
        [[serialize(4)]] FIELD(Asset::PrefabGameObjectFile) arrivalCaptionPrefab_;

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
            archive(CEREAL_NVP(arrivalOverview_msecs_));
            archive(CEREAL_NVP(arrivalOverviewDescend_msecs_));
            archive(CEREAL_NVP(arrivalOverviewCameraStart_));
            archive(CEREAL_NVP(arrivalOverviewCameraEnd_));
            archive(CEREAL_NVP(arrivalOverviewLookAt_));
            archive(CEREAL_NVP(arrivalIslandTitle_));
            archive(CEREAL_NVP(arrivalIslandSubtitle_));
            archive(CEREAL_NVP(arrivalTourShots_));
            archive(CEREAL_NVP(arrivalCaptionPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
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
            if (version <= 1)
            {
                // v0〜v1 は石・カメラ・パーティクルを別々に持っていた。今は石の FloatingStone が持つので読み捨てる
                [[serialize(0)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) oldStone;
                [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera)         oldStoneCamera;
                [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile)                   oldLiftOffParticle;
                [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile)                   oldFlightParticle;
                archive(cereal::make_nvp("floatingStone_",        oldStone));
                archive(cereal::make_nvp("floatingStoneCamera_",  oldStoneCamera));
                archive(cereal::make_nvp("stoneLiftOffParticle_", oldLiftOffParticle));
                archive(cereal::make_nvp("stoneFlightParticle_",  oldFlightParticle));
            }
            if (version == 1)
            {
                [[serialize(1)]] GamePlay::Prop::DepartShot oldDepartShot;
                archive(cereal::make_nvp("stoneDepartShot_", oldDepartShot));
            }
            if (version >= 2) archive(CEREAL_NVP(floatingStone_));
            if (version >= 3)
            {
                archive(CEREAL_NVP(arrivalOverview_msecs_));
                archive(CEREAL_NVP(arrivalOverviewDescend_msecs_));
                archive(CEREAL_NVP(arrivalOverviewCameraStart_));
                archive(CEREAL_NVP(arrivalOverviewCameraEnd_));
                archive(CEREAL_NVP(arrivalOverviewLookAt_));
            }
            if (version >= 4)
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
CEREAL_CLASS_VERSION(GameCore::Scene::DrySandSceneContext, 4);
#pragma endregion
