#include "Game.h"

#include "Scene/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "Scene/Group/Main/GameMainSceneGroup.h"
#include "Scene/Title/TitleScene.h"

GameCore::Game* GameCore::Game::instance_ = nullptr;

GameCore::Game::Game()
    : mainScenarioProgression_(std::make_unique<MainScenarioProgression>(MainScenarioProgression::TouchDownMainIsLand))
{
}

GameCore::Game::~Game() = default;

void GameCore::Game::OnAwake()
{
    sceneGroup_ = std::make_unique<Scene::GameMainSceneGroup>(sceneContexts_->Components().Catches<Scene::SceneContextBase>(), mainScenarioProgression_);
    sceneGroup_->RequestChangeScene<Scene::TitleScene>();
}

void GameCore::Game::OnUpdate()
{
    sceneGroup_->OnUpdate();
}

void GameCore::Game::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("sceneContexts_", sceneContexts_);
    ImGuiHelper::OnDrawInputField("sceneGroup_"   , sceneGroup_   );
    
    if (ImGui::Button("ChangeTitleScene"))
    {
        sceneGroup_->RequestChangeScene<Scene::TitleScene>();
    }
    if (ImGui::Button("ChangeMainIsLandScene"))
    {
        sceneGroup_->RequestChangeScene<Scene::FirstTouchDownMainIsLandScene>();
    }
}
