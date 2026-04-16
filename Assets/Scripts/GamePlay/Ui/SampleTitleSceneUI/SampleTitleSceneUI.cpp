#include "SampleTitleSceneUI.h"

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    void SampleTitleScene::OnStart()
    {
        gameStartButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            GameCore::Game::Instance().Scenes().RequestChangeScene<GameCore::Scene::Main::FirstTouchDownMainIsLandScene>();
        });
        gameExitButton_ ->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            
        });
    }
    
    void SampleTitleScene::OnUpdate()
    {
        
    }
    
    void SampleTitleScene::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("gameStartButton_", gameStartButton_);
        ImGuiHelper::OnDrawInputField("gameExitButton_" , gameExitButton_);
    }
}
