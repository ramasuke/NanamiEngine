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
    
    void TitleScene::OnInit()
    {
        // TitleSceneContext は GameManage 側に居るので、読み込みより先に触ってよい
        Context()->Init();
    }

    Coroutine::Task<EnterResult> TitleScene::OnEnterAsync(NanamiEngine::R4::CancellationToken)
    {
        co_return EnterResult::Ok();
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
