#pragma once
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Scene/SceneFile.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../../../../../Data/PlayerAvatar/InitStatus/SwordMan/Data_SwordManInitStatus.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Prop/AirShip/Prop_AirShip.h"
#include "../../../../../../../GamePlay/Prop/Canon/Prop_Canon.h"
#include "../../../../../../../GamePlay/Ui/PlayerStatus/Ui_PlayerStatus.h"
#include "../../../../../../../GamePlay/Ui/Sample/UI_SampleTitleLogo.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class FirstTouchDownMainIsLandSceneContext final : public SceneContextBase
    {
    public:
        void Init() override;
        std::shared_ptr<GamePlay::Prop::AirShip>                             AirShip()                                           { return airShip_.get(); }
        [[nodiscard]] GameObject::Transform&                                 AirShipFirstMoveFromTarget()                const   { return airShipFirstMoveFromTargetPos_->Transform(); }
        [[nodiscard]] GameObject::Transform&                                 AirShipSecondMoveFromTarget()               const   { return airShipSecondMoveFromTargetPos_->Transform(); }
        [[nodiscard]] int                                                    AirShipFirstMoveDuring_msecs ()             const   { return airShipFirstMoveDuring_msecs_;  }
        [[nodiscard]] int                                                    AirShipSecondMoveDuring_msecs()             const   { return airShipSecondMoveDuring_msecs_; }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile>           SummonPlayerAvatarPrefab()                          { return summonPlayerAvatarPrefab_.get(); }
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject>               OpeningShots()                                      { return openingShots_.get(); }
        [[nodiscard]] const std::vector<float>&                              OpeningShotDurations_secs()                 const   { return openingShotDurations_secs_; }
        [[nodiscard]] float                                                  HeroHoldRate()                              const   { return heroHoldRate_; }
        [[nodiscard]] float                                                  HeroTurnStartRate()                         const   { return heroTurnStartRate_; }
        [[nodiscard]] float                                                  HeroLookHeight()                            const   { return heroLookHeight_; }
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject>               AirShipDeckProps()                                  { return airShipDeckProps_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> SecondVirtualCamera()                               { return secondVirtualCamera_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CinemachineCameraBrain>   CameraBrain()                                       { return cameraBrain_.get(); }
        [[nodiscard]] GameObject::Transform&                                 PlayerFirstMoveTarget()                     const   { return playerFirstMoveTargetPos_->Transform(); }
        [[nodiscard]] int                                                    PlayerFirstMoveDuring_msecs()               const   { return playerFirstMoveDuring_msecs_; }
        [[nodiscard]] int                                                    PlayerArmStretchDuring_msecs()              const   { return playerArmStretchDuring_msecs_; }
        [[nodiscard]] std::weak_ptr<GamePlay::Ui::SampleTitleLogo>                     TitleLogo()                       const   { return titleLogo_.get(); }
        [[nodiscard]] std::shared_ptr<Asset::SoundFile>                                BGM() const { return bgm_.get(); }
        [[nodiscard]] const GameObject::IGameObject&                                   BoundryAirShipCollider() const { return *boundryAirshipCollider_.get(); }
        [[nodiscard]] const std::weak_ptr<Asset::PrefabGameObjectFile>&                FirstEventDragonPrefab() const { return firstEventDragonPrefab_.get(); }
        [[nodiscard]] const glm::vec3&                                                 FirstEventDragonSpawnPos () const { return firstEventDragonSpawnPos_->Transform().GetWorldPos(); }
        [[nodiscard]] GamePlay::Prop::Canon&                                           PlayerControllabeCanon   () const { return *playerControllabeCanon_.get(); }
        [[nodiscard]] bool                                                             HasPlayerControllabeCanon() const { return playerControllabeCanon_.get() != nullptr; }
        [[nodiscard]] Asset::PrefabGameObjectFile&                                     SwordManCameraGroupPrefab() const { return *swordManCameraGroupPrefab_.get(); }

    private:
        [[serialize(0)]] FIELD(GamePlay::Prop::AirShip)               airShip_;
        [[serialize(1)]] FIELD(GameObject::IGameObject)               airShipFirstMoveFromTargetPos_;
        [[serialize(1)]] int                                          airShipFirstMoveDuring_msecs_  = 0.0f;
        [[serialize(2)]] FIELD(GameObject::IGameObject)               airShipSecondMoveFromTargetPos_;
        [[serialize(2)]] int                                          airShipSecondMoveDuring_msecs_ = 0.0f;
        [[serialize(3)]] FIELD(Asset::PrefabGameObjectFile)           summonPlayerAvatarPrefab_;
        [[serialize(5)]] FIELD(CineMachine::CineMachineVirtualCamera) secondVirtualCamera_;
        [[serialize(5)]] FIELD(CineMachine::CinemachineCameraBrain)   cameraBrain_;
        [[serialize(6)]] FIELD(GameObject::IGameObject)               playerFirstMoveTargetPos_;
        [[serialize(7)]] int                                          playerFirstMoveDuring_msecs_ = 0;
        [[serialize(8)]] int                                          playerArmStretchDuring_msecs_ = 0;
        [[serialize(10)]] FIELD(GamePlay::Ui::SampleTitleLogo)        titleLogo_;
        [[serialize(13)]] FIELD(Asset::SoundFile)                     bgm_;
        [[serialize(13)]] FIELD(GameObject::IGameObject)              boundryAirshipCollider_;
        [[serialize(14)]] FIELD(Asset::PrefabGameObjectFile)          firstEventDragonPrefab_;
        [[serialize(14)]] FIELD(GameObject::IGameObject)              firstEventDragonSpawnPos_;
        [[serialize(16)]] FIELD(GamePlay::Prop::Canon)                playerControllabeCanon_;
        [[serialize(19)]] FIELD(Asset::PrefabGameObjectFile)          swordManCameraGroupPrefab_;
        /** 冒頭のカット。子の VirtualCamera を上から順に映し、その子(あれば)の位置・向きへ動かす */
        [[serialize(23)]] FIELD(GameObject::IGameObject)              openingShots_;
        [[serialize(23)]] std::vector<float>                          openingShotDurations_secs_;
        /** 甲板の小物。子孫の RigidBody は航行中 Kinematic で、着いたら Dynamic にする */
        [[serialize(24)]] FIELD(GameObject::IGameObject)              airShipDeckProps_;
        /** 最後のカット(主人公→追従カメラ)で、寄り始めるまで溜める割合 */
        [[serialize(25)]] float                                       heroHoldRate_      = 0.3f;
        /** 最後のカットで、追従カメラの向きへ振り向き始める割合 */
        [[serialize(25)]] float                                       heroTurnStartRate_ = 0.7f;
        /** 最後のカットで注視する、主人公の足元からの高さ */
        [[serialize(25)]] float                                       heroLookHeight_    = 12.0f;

#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<SceneContextBase>(this));
    archive(CEREAL_NVP(airShip_));
    archive(CEREAL_NVP(airShipFirstMoveFromTargetPos_));
    archive(CEREAL_NVP(airShipFirstMoveDuring_msecs_));
    archive(CEREAL_NVP(airShipSecondMoveFromTargetPos_));
    archive(CEREAL_NVP(airShipSecondMoveDuring_msecs_));
    archive(CEREAL_NVP(summonPlayerAvatarPrefab_));
    archive(CEREAL_NVP(secondVirtualCamera_));
    archive(CEREAL_NVP(cameraBrain_));
    archive(CEREAL_NVP(playerFirstMoveTargetPos_));
    archive(CEREAL_NVP(playerFirstMoveDuring_msecs_));
    archive(CEREAL_NVP(playerArmStretchDuring_msecs_));
    archive(CEREAL_NVP(titleLogo_));
    [[serialize(11)]] FIELD(GameObject::IGameObject) actionControlWayUi_;
    if (version <= 20) archive(CEREAL_NVP(actionControlWayUi_));
    [[serialize(12)]] FIELD(GamePlay::Ui::PlayerStatus) playerStatusUi_;
    if (version <= 19) archive(CEREAL_NVP(playerStatusUi_));
    archive(CEREAL_NVP(bgm_));
    archive(CEREAL_NVP(boundryAirshipCollider_));
    archive(CEREAL_NVP(firstEventDragonPrefab_));
    archive(CEREAL_NVP(firstEventDragonSpawnPos_));
    archive(CEREAL_NVP(playerControllabeCanon_));
    archive(CEREAL_NVP(swordManCameraGroupPrefab_));
    archive(CEREAL_NVP(openingShots_));
    archive(CEREAL_NVP(openingShotDurations_secs_));
    archive(CEREAL_NVP(airShipDeckProps_));
    archive(CEREAL_NVP(heroHoldRate_));
    archive(CEREAL_NVP(heroTurnStartRate_));
    archive(CEREAL_NVP(heroLookHeight_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<SceneContextBase>(this));
    if (version >= 0) archive(CEREAL_NVP(airShip_));
    if (version >= 1) archive(CEREAL_NVP(airShipFirstMoveFromTargetPos_));
    if (version >= 1) archive(CEREAL_NVP(airShipFirstMoveDuring_msecs_));
    if (version >= 2) archive(CEREAL_NVP(airShipSecondMoveFromTargetPos_));
    if (version >= 2) archive(CEREAL_NVP(airShipSecondMoveDuring_msecs_));
    if (version >= 3) archive(CEREAL_NVP(summonPlayerAvatarPrefab_));
    [[serialize(4)]] FIELD(CineMachine::CineMachineVirtualCamera) firstVirtualCamera_;
    [[serialize(4)]] FIELD(GameObject::IGameObject) virtualCameraFirstMoveTarget_;
    [[serialize(4)]] int virtualCameraFirstMoveTargetDuring_msecs_ = 0;
    if (version >= 4 && version <= 22) archive(CEREAL_NVP(firstVirtualCamera_));
    if (version >= 4 && version <= 22) archive(CEREAL_NVP(virtualCameraFirstMoveTarget_));
    if (version >= 4 && version <= 22) archive(CEREAL_NVP(virtualCameraFirstMoveTargetDuring_msecs_));
    if (version >= 5) archive(CEREAL_NVP(secondVirtualCamera_));
    if (version >= 5) archive(CEREAL_NVP(cameraBrain_));
    if (version >= 6) archive(CEREAL_NVP(playerFirstMoveTargetPos_));
    if (version >= 7) archive(CEREAL_NVP(playerFirstMoveDuring_msecs_));
    if (version >= 8) archive(CEREAL_NVP(playerArmStretchDuring_msecs_));
    if (version >= 10) archive(CEREAL_NVP(titleLogo_));
    [[serialize(11)]] FIELD(GameObject::IGameObject) actionControlWayUi_;
    if (version >= 11 && version <= 20) archive(CEREAL_NVP(actionControlWayUi_));
    [[serialize(12)]] FIELD(GamePlay::Ui::PlayerStatus) playerStatusUi_;
    if (version <= 19) archive(CEREAL_NVP(playerStatusUi_));
    if (version >= 13) archive(CEREAL_NVP(bgm_));
    if (version >= 13) archive(CEREAL_NVP(boundryAirshipCollider_));
    if (version >= 14) archive(CEREAL_NVP(firstEventDragonPrefab_));
    if (version >= 14) archive(CEREAL_NVP(firstEventDragonSpawnPos_));
    FIELD(Asset::SwordManInitStatus) playerAvatarInitStatus_;
    if (version <= 21) archive(CEREAL_NVP(playerAvatarInitStatus_));
    if (version >= 16) archive(CEREAL_NVP(playerControllabeCanon_));
    if (version >= 19) archive(CEREAL_NVP(swordManCameraGroupPrefab_));
    if (version >= 23) archive(CEREAL_NVP(openingShots_));
    if (version >= 23) archive(CEREAL_NVP(openingShotDurations_secs_));
    if (version >= 24) archive(CEREAL_NVP(airShipDeckProps_));
    if (version >= 25) archive(CEREAL_NVP(heroHoldRate_));
    if (version >= 25) archive(CEREAL_NVP(heroTurnStartRate_));
    if (version >= 25) archive(CEREAL_NVP(heroLookHeight_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::FirstTouchDownMainIsLandSceneContext, 25);
#pragma endregion
