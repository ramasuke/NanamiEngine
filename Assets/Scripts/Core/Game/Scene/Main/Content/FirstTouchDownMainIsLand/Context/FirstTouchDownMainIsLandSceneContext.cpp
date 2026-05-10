#include "FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene
{
    void FirstTouchDownMainIsLandSceneContext::Init()
    {
        SceneContextBase::Init();
        airShip_                       .Init();
        airShipFirstMoveFromTargetPos_ .Init();
        airShipSecondMoveFromTargetPos_.Init();
        firstVirtualCamera_            .Init();
        virtualCameraFirstMoveTarget_  .Init();
        secondVirtualCamera_           .Init();
        cameraBrain_                   .Init();
        playerFirstMoveTargetPos_      .Init();
        cameraGroup_                   .Init();
        titleLogo_                     .Init();
        actionControlWayUi_            .Init();
        playerStatusUi_                .Init();
        boundryAirshipCollider_        .Init();
        firstEventDragonPrefab_        .Init();
        firstEventDragonSpawnPos_      .Init();
        playerControllabeCanon_        .Init();
    }
    
    void FirstTouchDownMainIsLandSceneContext::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("airShip_", airShip_);
        LibCore::ImGuiHelper::OnDrawInputField("airShipFirstMoveFromTargetPos_", airShipFirstMoveFromTargetPos_);
        LibCore::ImGuiHelper::OnDrawInputField("airShipFirstMoveDuring_msecs_", airShipFirstMoveDuring_msecs_);
        LibCore::ImGuiHelper::OnDrawInputField("airShipSecondMoveFromTargetPos_", airShipSecondMoveFromTargetPos_);
        LibCore::ImGuiHelper::OnDrawInputField("airShipSecondMoveDuring_msecs_", airShipSecondMoveDuring_msecs_);
        LibCore::ImGuiHelper::OnDrawInputField("summonPlayerAvatarPrefab_", summonPlayerAvatarPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("firstVirtualCamera_", firstVirtualCamera_);
        LibCore::ImGuiHelper::OnDrawInputField("virtualCameraFirstMoveTarget_", virtualCameraFirstMoveTarget_);
        LibCore::ImGuiHelper::OnDrawInputField("virtualCameraFirstMoveTargetDuring_msecs_", virtualCameraFirstMoveTargetDuring_msecs_);
        LibCore::ImGuiHelper::OnDrawInputField("secondVirtualCamera_", secondVirtualCamera_);
        LibCore::ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
        LibCore::ImGuiHelper::OnDrawInputField("playerFirstMoveTargetPos_", playerFirstMoveTargetPos_);
        LibCore::ImGuiHelper::OnDrawInputField("playerFirstMoveDuring_msecs_", playerFirstMoveDuring_msecs_);
        LibCore::ImGuiHelper::OnDrawInputField("playerArmStretchDuring_msecs_", playerArmStretchDuring_msecs_);
        LibCore::ImGuiHelper::OnDrawInputField("cameraGroup_", cameraGroup_);
        LibCore::ImGuiHelper::OnDrawInputField("titleLogo_", titleLogo_);
        LibCore::ImGuiHelper::OnDrawInputField("actionControlWayUi_", actionControlWayUi_);
        LibCore::ImGuiHelper::OnDrawInputField("playerStatusUi_", playerStatusUi_);
        LibCore::ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        LibCore::ImGuiHelper::OnDrawInputField("boundryAirshipCollider_", boundryAirshipCollider_);
        LibCore::ImGuiHelper::OnDrawInputField("firstEventDragonPrefab_", firstEventDragonPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("firstEventDragonSpawnPos_", firstEventDragonSpawnPos_);
        LibCore::ImGuiHelper::OnDrawInputField("playerAvatarInitStatus_", playerAvatarInitStatus_);
        LibCore::ImGuiHelper::OnDrawInputField("playerControllabeCanon_", playerControllabeCanon_);
    }
}