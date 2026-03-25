#include "TitleScene.h"

#include "../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../GamePlay/Sound/SoundPlayer.h"

GameCore::Scene::TitleScene::TitleScene(const std::weak_ptr<TitleSceneContext>& context, const GameSceneBaseContext baseContext)
    : GameSceneBase(context, std::move(baseContext))
{
}

void GameCore::Scene::TitleScene::Init()
{
    Context()->Init();
    scene_ = OnLoadMainScene();
}

void GameCore::Scene::TitleScene::OnEnter()
{
    
}

void GameCore::Scene::TitleScene::OnExit()
{
    GamePlay::Sound::SoundPlayer::StopAllBgm();
}

void GameCore::Scene::TitleScene::Dispose()
{
    Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
}

void GameCore::Scene::TitleScene::OnDrawGui()
{
    
}
