#include "FirstTouchDownMainIsLandSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene
{
    void FirstTouchDownMainIsLandSceneContext::Init()
    {
        SceneContextBase::Init();
        airShip_                       .Init();
        airShipFirstMoveFromTargetPos_ .Init();
        airShipSecondMoveFromTargetPos_.Init();
        secondVirtualCamera_           .Init();
        cameraBrain_                   .Init();
        playerFirstMoveTargetPos_      .Init();
        titleLogo_                     .Init();
        boundryAirshipCollider_        .Init();
        firstEventDragonPrefab_        .Init();
        firstEventDragonSpawnPos_      .Init();
        playerControllabeCanon_        .Init();
        swordManCameraGroupPrefab_     .Init();
        openingShots_                  .Init();
        airShipDeckProps_              .Init();
        bgm_                           .Init();
    }
    
    void FirstTouchDownMainIsLandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("airShip_", airShip_);
        ImGuiHelper::OnDrawInputField("airShipFirstMoveFromTargetPos_", airShipFirstMoveFromTargetPos_);
        ImGuiHelper::OnDrawInputField("airShipFirstMoveDuring_msecs_", airShipFirstMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("airShipSecondMoveFromTargetPos_", airShipSecondMoveFromTargetPos_);
        ImGuiHelper::OnDrawInputField("airShipSecondMoveDuring_msecs_", airShipSecondMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("summonPlayerAvatarPrefab_", summonPlayerAvatarPrefab_);
        ImGuiHelper::OnDrawInputField("secondVirtualCamera_", secondVirtualCamera_);
        ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
        ImGuiHelper::OnDrawInputField("playerFirstMoveTargetPos_", playerFirstMoveTargetPos_);
        ImGuiHelper::OnDrawInputField("playerFirstMoveDuring_msecs_", playerFirstMoveDuring_msecs_);
        ImGuiHelper::OnDrawInputField("playerArmStretchDuring_msecs_", playerArmStretchDuring_msecs_);
        ImGuiHelper::OnDrawInputField("titleLogo_", titleLogo_);
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("boundryAirshipCollider_", boundryAirshipCollider_);
        ImGuiHelper::OnDrawInputField("firstEventDragonPrefab_", firstEventDragonPrefab_);
        ImGuiHelper::OnDrawInputField("firstEventDragonSpawnPos_", firstEventDragonSpawnPos_);
        ImGuiHelper::OnDrawInputField("playerControllabeCanon_", playerControllabeCanon_);
        ImGuiHelper::OnDrawInputField("swordManCameraGroupPrefab_", swordManCameraGroupPrefab_);
        ImGuiHelper::OnDrawInputField("openingShots_", openingShots_);
        ImGuiHelper::OnDrawInputField("openingShotDurations_secs_", openingShotDurations_secs_, [this]
        {
            if (ImGui::Button("Add"))
                openingShotDurations_secs_.push_back(4.0f);
        });
        ImGuiHelper::OnDrawInputField("airShipDeckProps_", airShipDeckProps_);
        ImGuiHelper::OnDrawInputField("heroHoldRate_", heroHoldRate_);
        ImGuiHelper::OnDrawInputField("heroTurnStartRate_", heroTurnStartRate_);
        ImGuiHelper::OnDrawInputField("heroLookHeight_", heroLookHeight_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::FirstTouchDownMainIsLandSceneContext, GameCore::Scene::SceneContextBase);
#pragma endregion
