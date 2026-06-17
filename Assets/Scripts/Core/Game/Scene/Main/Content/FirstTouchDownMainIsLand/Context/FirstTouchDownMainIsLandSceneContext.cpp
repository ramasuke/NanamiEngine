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
        //swordmanCameraGroup_           .Init();
        titleLogo_                     .Init();
        actionControlWayUi_            .Init();
        playerStatusUi_                .Init();
        boundryAirshipCollider_        .Init();
        firstEventDragonPrefab_        .Init();
        firstEventDragonSpawnPos_      .Init();
        playerControllabeCanon_        .Init();
        swordManCameraGroupPrefab_     .Init();
    }
    
    void FirstTouchDownMainIsLandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("airShip_", airShip_);
        ImGuiHelper::OnDrawInputField("airShipFirstMoveFromTargetPos_", airShipFirstMoveFromTargetPos_);
        ImGuiHelper::OnDrawInputField("airShipFirstMoveDuring_msecs_", airShipFirstMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("airShipSecondMoveFromTargetPos_", airShipSecondMoveFromTargetPos_);
        ImGuiHelper::OnDrawInputField("airShipSecondMoveDuring_msecs_", airShipSecondMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("summonPlayerAvatarPrefab_", summonPlayerAvatarPrefab_);
        ImGuiHelper::OnDrawInputField("firstVirtualCamera_", firstVirtualCamera_);
        ImGuiHelper::OnDrawInputField("virtualCameraFirstMoveTarget_", virtualCameraFirstMoveTarget_);
        ImGuiHelper::OnDrawInputField("virtualCameraFirstMoveTargetDuring_msecs_", virtualCameraFirstMoveTargetDuring_msecs_);
        ImGuiHelper::OnDrawInputField("secondVirtualCamera_", secondVirtualCamera_);
        ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
        ImGuiHelper::OnDrawInputField("playerFirstMoveTargetPos_", playerFirstMoveTargetPos_);
        ImGuiHelper::OnDrawInputField("playerFirstMoveDuring_msecs_", playerFirstMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("playerArmStretchDuring_msecs_", playerArmStretchDuring_msecs_);
        //LibCore::ImGuiHelper::OnDrawInputField("cameraGroup_", swordmanCameraGroup_);
        ImGuiHelper::OnDrawInputField("titleLogo_", titleLogo_);
        ImGuiHelper::OnDrawInputField("actionControlWayUi_", actionControlWayUi_);
        ImGuiHelper::OnDrawInputField("playerStatusUi_", playerStatusUi_);
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("boundryAirshipCollider_", boundryAirshipCollider_);
        ImGuiHelper::OnDrawInputField("firstEventDragonPrefab_", firstEventDragonPrefab_);
        ImGuiHelper::OnDrawInputField("firstEventDragonSpawnPos_", firstEventDragonSpawnPos_);
        ImGuiHelper::OnDrawInputField("playerAvatarInitStatus_", playerAvatarInitStatus_);
        ImGuiHelper::OnDrawInputField("playerControllabeCanon_", playerControllabeCanon_);
        ImGuiHelper::OnDrawInputField("swordManCameraGroupPrefab_", swordManCameraGroupPrefab_);
    }
}