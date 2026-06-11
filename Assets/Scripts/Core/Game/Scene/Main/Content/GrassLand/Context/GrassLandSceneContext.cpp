#include "GrassLandSceneContext.h"

void GameCore::Scene::GrassLandSceneContext::Init()
{
    SceneContextBase::Init();
    
    bgm_.Init();
    networkRunner_.Init();
}

void GameCore::Scene::GrassLandSceneContext::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    ImGuiHelper::OnDrawInputField("networkRunner_", networkRunner_);
}
