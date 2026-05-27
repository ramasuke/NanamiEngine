#include "GrassLandSceneContext.h"

void GameCore::Scene::GrassLandSceneContext::Init()
{
    SceneContextBase::Init();
    
    bgm_.Init();
}

void GameCore::Scene::GrassLandSceneContext::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("bgm_", bgm_);
}
