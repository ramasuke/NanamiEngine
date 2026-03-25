#include "SampleTitleSceneUI.h"

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../../Core/Game/Scene/Group/Main/GameMainSceneGroup.h"

namespace GamePlay::Ui
{
    void SampleTitleScene::OnStart()
    {
        gameStartButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            GameCore::Game::Instance().MainScene().RequestChangeScene<GameCore::Scene::FirstTouchDownMainIsLandScene>();
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
