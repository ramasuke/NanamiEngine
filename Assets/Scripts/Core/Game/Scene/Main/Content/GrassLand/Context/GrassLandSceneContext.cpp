#include "GrassLandSceneContext.h"

#include "../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"

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

std::vector<std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>>
GameCore::Scene::GrassLandSceneContext::EnemySpawnPoints() const
{
    return enemySpawnPointsRoot_->Transform().GetAllChildren();
}

void GameCore::Scene::GrassLandSceneContext::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    ImGuiHelper::OnDrawInputField("networkRunner_", networkRunner_);
    ImGuiHelper::OnDrawInputField("enemySpawnPointsRoot_", enemySpawnPointsRoot_);
    ImGuiHelper::OnDrawEnumField("enemyKind_", enemyKind_, Npc::Enemy::ENEMY_KINDS, Npc::Enemy::ToString);
    ImGuiHelper::OnDrawInputField("arrivalCamera_", arrivalCamera_);
    ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
    ImGuiHelper::OnDrawInputField("arrivalPortalPrefab_", arrivalPortalPrefab_);
    ImGuiHelper::OnDrawInputField("arrivalShotDuring_msecs_", arrivalShotDuring_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalShotStart_", arrivalShotStart_);
    ImGuiHelper::OnDrawInputField("arrivalShotEnd_", arrivalShotEnd_);
    ImGuiHelper::OnDrawInputField("arrivalLookAtOffsetStart_", arrivalLookAtOffsetStart_);
    ImGuiHelper::OnDrawInputField("arrivalLookAtOffsetEnd_", arrivalLookAtOffsetEnd_);
}
