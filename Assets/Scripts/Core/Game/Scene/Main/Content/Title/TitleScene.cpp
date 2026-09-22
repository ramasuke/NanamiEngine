#include "TitleScene.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Scene::Main
{
    TitleScene::TitleScene(const std::weak_ptr<TitleSceneContext>& context, const GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, std::move(baseContext))
    {
    }
    
    void TitleScene::Init()
    {
        // TitleSceneContext は GameManage 側に居るので、読み込みより先に触ってよい
        Context()->Init();
        Coroutine::StartCoroutine(OnEnterAsync(BeginEnter()));
    }

    Coroutine::Task<void> TitleScene::OnEnterAsync(const int generation)
    {
        if (!co_await LoadMainSceneAsync(generation))
            co_return;

        CompleteEnter(generation);
    }
    
    void TitleScene::Enter()
    {
        
    }
    
    void TitleScene::DoDispose()
    {
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }
    
    void TitleScene::OnDrawGui()
    {
        
    }
}
