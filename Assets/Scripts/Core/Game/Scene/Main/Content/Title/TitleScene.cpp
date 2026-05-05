#include "TitleScene.h"

#include "../../../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Scene::Main
{
    TitleScene::TitleScene(const std::weak_ptr<TitleSceneContext>& context, const GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, std::move(baseContext))
    {
    }
    
    void TitleScene::Init()
    {
        Context()->Init();
        scene_ = LoadMainScene();
    }
    
    void TitleScene::OnEnter()
    {
        
    }
    
    void TitleScene::OnExit()
    {
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }
    
    void TitleScene::Dispose()
    {
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
    }
    
    void TitleScene::OnDrawGui()
    {
        
    }
}
