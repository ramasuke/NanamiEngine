#include "GrassLandSceneContext.h"

#include "../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"

void GameCore::Scene::GrassLandSceneContext::Init()
{
    SceneContextBase::Init();

    bgm_.Init();
    networkRunner_.Init();
    enemyPrefab_.Init();
    enemySpawnPointsRoot_.Init();
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
    ImGuiHelper::OnDrawInputField("enemyPrefab_", enemyPrefab_);
    ImGuiHelper::OnDrawInputField("enemySpawnPointsRoot_", enemySpawnPointsRoot_);
}
