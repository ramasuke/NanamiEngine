#pragma once
#include "../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../Data/PlayerAvatarInitStatus/SwordMan/Data_SwordManInitStatus.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Ui/PlayerStatus/Ui_PlayerStatus.h"
#include "../../../../../../../GamePlay/Ui/Sample/SampleTitleLogo.h"
#include "../../../Context/Main_SceneContextBase.h"

namespace GameCore::Scene
{
    class FirstTouchDownMainIsLandSceneContext final : public SceneContextBase
    {
    public:
        using SummonAvatarTraits = PlayerAvatar::SwordMan::SwordManAvatarTraits;
        using SummonAvatarStatus = PlayerAvatar::RequireType::Status<SummonAvatarTraits>;

    public:
        void Init() override;
        std::shared_ptr<GameObject::IGameObject>                             AirShip()                                           { return airShip_.get();                        }
        [[nodiscard]] GameObject::Transform&                                 AirShipFirstMoveFromTarget()                const   { return airShipFirstMoveFromTargetPos_->TransformRef(); }
        [[nodiscard]] GameObject::Transform&                                 AirShipSecondMoveFromTarget()               const   { return airShipSecondMoveFromTargetPos_->TransformRef(); }
        [[nodiscard]] int                                                    AirShipFirstMoveDuring_msecs ()             const   { return airShipFirstMoveDuring_msecs_;  }
        [[nodiscard]] int                                                    AirShipSecondMoveDuring_msecs()             const   { return airShipSecondMoveDuring_msecs_; }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile>           SummonPlayerAvatarPrefab()                          { return summonPlayerAvatarPrefab_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> FirstVirtualCamera()                                { return firstVirtualCamera_.get(); }
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject>               VirtualCameraFirstMoveTarget()                      { return virtualCameraFirstMoveTarget_.get(); }
        [[nodiscard]] int                                                    VirtualCameraFirstMoveTargetDuring_msecs()  const   { return virtualCameraFirstMoveTargetDuring_msecs_; }
        [[nodiscard]] std::shared_ptr<CineMachine::CineMachineVirtualCamera> SecondVirtualCamera()                               { return secondVirtualCamera_.get(); }
        [[nodiscard]] std::shared_ptr<CineMachine::CinemachineCameraBrain>   CameraBrain()                                       { return cameraBrain_.get(); }
        [[nodiscard]] GameObject::Transform&                                 PlayerFirstMoveTarget()                     const   { return playerFirstMoveTargetPos_->TransformRef(); }
        [[nodiscard]] int                                                    PlayerFirstMoveDuring_msecs()               const   { return playerFirstMoveDuring_msecs_; }
        [[nodiscard]] int                                                    PlayerArmStretchDuring_msecs()              const   { return playerArmStretchDuring_msecs_; }
        [[nodiscard]] std::weak_ptr<PlayerAvatar::SwordMan::SwordManAvatarCameraGroup> CameraGroup()                     const   { return cameraGroup_.get(); }
        [[nodiscard]] std::weak_ptr<GamePlay::Ui::SampleTitleLogo>                     TitleLogo()                       const   { return titleLogo_.get(); }
        [[nodiscard]] const std::weak_ptr<GameObject::IGameObject>&                    ActionControlWayUI()              const   { return actionControlWayUi_.get(); }
        [[nodiscard]] const std::weak_ptr<GamePlay::Ui::PlayerStatus>&                 PlayerStatusUI()                  const   { return playerStatusUi_.get(); }
        [[nodiscard]] const std::weak_ptr<Asset::SoundFile>&                           BGM() const { return bgm_.get(); }
        [[nodiscard]] const GameObject::IGameObject&                                   BoundryAirShipCollider() const { return *boundryAirshipCollider_.get(); }
        [[nodiscard]] const std::weak_ptr<Asset::PrefabGameObjectFile>&                FirstEventDragonPrefab() const { return firstEventDragonPrefab_.get(); }
        [[nodiscard]] const glm::vec3&                                                 FirstEventDragonSpawnPos() const { return firstEventDragonSpawnPos_->TransformRef().GetWorldPos(); }
        [[nodiscard]] const Asset::SwordManInitStatus&                                 PlayerAvatarInitStatus  () const { return *playerAvatarInitStatus_.get(); }
        
    private:
        [[serialize(0)]] FIELD(GameObject::IGameObject)               airShip_;
        [[serialize(1)]] FIELD(GameObject::IGameObject)               airShipFirstMoveFromTargetPos_;
        [[serialize(1)]] int                                          airShipFirstMoveDuring_msecs_  = 0.0f;
        [[serialize(2)]] FIELD(GameObject::IGameObject)               airShipSecondMoveFromTargetPos_;
        [[serialize(2)]] int                                          airShipSecondMoveDuring_msecs_ = 0.0f;
        [[serialize(3)]] FIELD(Asset::PrefabGameObjectFile)           summonPlayerAvatarPrefab_;
        [[serialize(4)]] FIELD(CineMachine::CineMachineVirtualCamera) firstVirtualCamera_;
        [[serialize(4)]] FIELD(GameObject::IGameObject)               virtualCameraFirstMoveTarget_;
        [[serialize(4)]] int                                          virtualCameraFirstMoveTargetDuring_msecs_ = 0;
        [[serialize(5)]] FIELD(CineMachine::CineMachineVirtualCamera) secondVirtualCamera_;
        [[serialize(5)]] FIELD(CineMachine::CinemachineCameraBrain)   cameraBrain_;
        [[serialize(6)]] FIELD(GameObject::IGameObject)               playerFirstMoveTargetPos_;
        [[serialize(7)]] int                                          playerFirstMoveDuring_msecs_ = 0;
        [[serialize(8)]] int                                          playerArmStretchDuring_msecs_ = 0;
        [[serialize(9)]] FIELD(PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) cameraGroup_;
        [[serialize(10)]] FIELD(GamePlay::Ui::SampleTitleLogo)        titleLogo_;
        [[serialize(11)]] FIELD(GameObject::IGameObject)              actionControlWayUi_;
        [[serialize(12)]] FIELD(GamePlay::Ui::PlayerStatus)           playerStatusUi_;
        [[serialize(13)]] FIELD(Asset::SoundFile)                     bgm_;
        [[serialize(13)]] FIELD(GameObject::IGameObject)              boundryAirshipCollider_;
        [[serialize(14)]] FIELD(Asset::PrefabGameObjectFile)          firstEventDragonPrefab_;
        [[serialize(14)]] FIELD(GameObject::IGameObject)              firstEventDragonSpawnPos_;
        [[serialize(15)]] FIELD(Asset::SwordManInitStatus)            playerAvatarInitStatus_;
        
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
    archive(CEREAL_NVP(firstVirtualCamera_));
    archive(CEREAL_NVP(virtualCameraFirstMoveTarget_));
    archive(CEREAL_NVP(virtualCameraFirstMoveTargetDuring_msecs_));
    archive(CEREAL_NVP(secondVirtualCamera_));
    archive(CEREAL_NVP(cameraBrain_));
    archive(CEREAL_NVP(playerFirstMoveTargetPos_));
    archive(CEREAL_NVP(playerFirstMoveDuring_msecs_));
    archive(CEREAL_NVP(playerArmStretchDuring_msecs_));
    archive(CEREAL_NVP(cameraGroup_));
    archive(CEREAL_NVP(titleLogo_));
    archive(CEREAL_NVP(actionControlWayUi_));
    archive(CEREAL_NVP(playerStatusUi_));
    archive(CEREAL_NVP(bgm_));
    archive(CEREAL_NVP(boundryAirshipCollider_));
    archive(CEREAL_NVP(firstEventDragonPrefab_));
    archive(CEREAL_NVP(firstEventDragonSpawnPos_));
    archive(CEREAL_NVP(playerAvatarInitStatus_));
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
    if (version >= 4) archive(CEREAL_NVP(firstVirtualCamera_));
    if (version >= 4) archive(CEREAL_NVP(virtualCameraFirstMoveTarget_));
    if (version >= 4) archive(CEREAL_NVP(virtualCameraFirstMoveTargetDuring_msecs_));
    if (version >= 5) archive(CEREAL_NVP(secondVirtualCamera_));
    if (version >= 5) archive(CEREAL_NVP(cameraBrain_));
    if (version >= 6) archive(CEREAL_NVP(playerFirstMoveTargetPos_));
    if (version >= 7) archive(CEREAL_NVP(playerFirstMoveDuring_msecs_));
    if (version >= 8) archive(CEREAL_NVP(playerArmStretchDuring_msecs_));
    if (version >= 9) archive(CEREAL_NVP(cameraGroup_));
    if (version >= 10) archive(CEREAL_NVP(titleLogo_));
    if (version >= 11) archive(CEREAL_NVP(actionControlWayUi_));
    if (version >= 12) archive(CEREAL_NVP(playerStatusUi_));
    if (version >= 13) archive(CEREAL_NVP(bgm_));
    if (version >= 13) archive(CEREAL_NVP(boundryAirshipCollider_));
    if (version >= 14) archive(CEREAL_NVP(firstEventDragonPrefab_));
    if (version >= 14) archive(CEREAL_NVP(firstEventDragonSpawnPos_));
    if (version >= 15) archive(CEREAL_NVP(playerAvatarInitStatus_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::FirstTouchDownMainIsLandSceneContext, 15);
CEREAL_REGISTER_TYPE(GameCore::Scene::FirstTouchDownMainIsLandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::FirstTouchDownMainIsLandSceneContext);
#pragma endregion
