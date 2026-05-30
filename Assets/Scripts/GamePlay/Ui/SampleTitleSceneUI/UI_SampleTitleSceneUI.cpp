#include "UI_SampleTitleSceneUI.h"

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    void SampleTitleScene::OnStart()
    {
        gameStartButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            OnGameStart();
        });
        gameExitButton_ ->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            
        });
    }
    
    void SampleTitleScene::OnUpdate()
    {
        
    }

    void SampleTitleScene::OnGameStart()
    {
        switch (GameCore::LoadGameProgression())
        {
        case GameCore::GameProgresion::FirstTouchDownMainIsLand:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::FirstTouchDownMainIsLand);
            break;
        case GameCore::GameProgresion::MainIsland:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::MainIsland);
            break;
        default: 
            throw std::runtime_error("Gameの進行状況に応じたScene遷移が定義されていません。");
        }
    }

    void SampleTitleScene::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("gameStartButton_", gameStartButton_);
        ImGuiHelper::OnDrawInputField("gameExitButton_" , gameExitButton_);
    }
}
