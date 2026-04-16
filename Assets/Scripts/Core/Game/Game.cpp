#include "Game.h"

#include "Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "Scene/Main/Group/Main_GameSceneGroup.h"
#include "Scene/Main/Content/Title/TitleScene.h"
#include "Scene/Sub/Context/Sub_SceneContextBase.h"
#include "Scene/Sub/Group/Sub_GameSceneGroup.h"

namespace GameCore
{
    Game* Game::instance_ = nullptr;
    
    Game::Game()
        : mainScenarioProgression_(std::make_shared<MainScenarioProgression>(MainScenarioProgression::TouchDownMainIsLand))
    {
    }
    
    Game::~Game() = default;
    
    Scene::Sub::GameSceneGroup& Game::SubScenes() const
    {
        return *subSceneGroup_;
    }
    
    void Game::OnAwake()
    {
        subSceneGroup_ = std::make_shared<Scene::Sub::GameSceneGroup>(subSceneContexts_->Components().Catches<Scene::Sub::SceneContextBase>());
        
        sceneGroup_ = std::make_unique<Scene::Main::GameSceneGroup>(
            sceneContexts_->Components().Catches<Scene::SceneContextBase>(),
            mainScenarioProgression_,
            subSceneGroup_);
        sceneGroup_->RequestChangeScene<Scene::Main::TitleScene>();
    }
    
    void Game::OnUpdate()
    {
        sceneGroup_->OnUpdate();
    }
    
    void Game::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sceneContexts_", sceneContexts_);
        ImGuiHelper::OnDrawInputField("sceneGroup_"   , sceneGroup_   );
        
        if (ImGui::Button("ChangeTitleScene"))
        {
            sceneGroup_->RequestChangeScene<Scene::Main::TitleScene>();
        }
        if (ImGui::Button("ChangeMainIsLandScene"))
        {
            sceneGroup_->RequestChangeScene<Scene::Main::FirstTouchDownMainIsLandScene>();
        }

        ImGuiHelper::OnDrawInputField("subSceneGroup_"      , subSceneGroup_   );
        ImGuiHelper::OnDrawInputField("subSceneContexts_"   , subSceneContexts_);
    }
}
