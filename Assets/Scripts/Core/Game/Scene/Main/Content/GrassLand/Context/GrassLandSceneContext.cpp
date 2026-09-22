#include "GrassLandSceneContext.h"

#include "Engine/Module/GameObject/Transform/Transform.h"

void GameCore::Scene::GrassLandSceneContext::Init()
{
    SceneContextBase::Init();

    bgm_.Init();
    networkRunner_.Init();
    enemySpawnPointsRoot_.Init();
    arrivalCamera_.Init();
    cameraBrain_.Init();
    arrivalPortalPrefab_.Init();
}

std::vector<std::shared_ptr<GameCore::Npc::Enemy::EnemySpawnPoint>>
GameCore::Scene::GrassLandSceneContext::EnemySpawnPoints() const
{
    std::vector<std::shared_ptr<Npc::Enemy::EnemySpawnPoint>> spawnPoints;
    for (const auto& child : enemySpawnPointsRoot_->Transform().GetAllChildren())
    {
        if (const auto spawnPoint = child->Components().Catch<Npc::Enemy::EnemySpawnPoint>().lock())
            spawnPoints.push_back(spawnPoint);
    }
    return spawnPoints;
}

void GameCore::Scene::GrassLandSceneContext::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    ImGuiHelper::OnDrawInputField("networkRunner_", networkRunner_);
    ImGuiHelper::OnDrawInputField("enemySpawnPointsRoot_", enemySpawnPointsRoot_);
    ImGuiHelper::OnDrawInputField("arrivalCamera_", arrivalCamera_);
    ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
    ImGuiHelper::OnDrawInputField("arrivalPortalPrefab_", arrivalPortalPrefab_);
    ImGuiHelper::OnDrawInputField("arrivalPortalOpenDelay_msecs_", arrivalPortalOpenDelay_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalOpen_msecs_", arrivalPortalOpen_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalWalk_msecs_", arrivalWalk_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalCloseDelay_msecs_", arrivalPortalCloseDelay_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalClose_msecs_", arrivalPortalClose_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalHold_msecs_", arrivalHold_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalHeight_", arrivalPortalHeight_);
    ImGuiHelper::OnDrawInputField("arrivalWalkStartBehind_", arrivalWalkStartBehind_);
    ImGuiHelper::OnDrawInputField("arrivalWalkDistance_", arrivalWalkDistance_);
    ImGuiHelper::OnDrawInputField("arrivalCameraStart_", arrivalCameraStart_);
    ImGuiHelper::OnDrawInputField("arrivalCameraEnd_", arrivalCameraEnd_);
    ImGuiHelper::OnDrawInputField("arrivalLookAtHeight_", arrivalLookAtHeight_);
}
