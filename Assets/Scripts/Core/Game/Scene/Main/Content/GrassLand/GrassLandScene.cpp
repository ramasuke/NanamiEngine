#include "GrassLandScene.h"

#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"

namespace GameCore::Scene::Main
{
    GrassLandScene::GrassLandScene(
        const std::weak_ptr<GrassLandSceneContext>& context,
        GameSceneBaseContext baseContext)
            : GameMainSceneBase(context, baseContext)
    {
    }

    GrassLandScene::~GrassLandScene() = default;

    void GrassLandScene::Init()
    {
        if (!Context())
        {
            throw std::runtime_error("GrassLandSceneContextが設定されていません。GameManage.sceneにGrassLandSceneContextを追加してください。");
        }

        SubScene().Push(Sub::SceneType::ChattingUI);
        
        scene_ = LoadMainScene();
        Context()->Init();

        Context()->NetworkRunner().Initialize();
        Coroutine::StartCoroutine(OnEnterAsync());
    }

    Coroutine::Task<void> GrassLandScene::OnEnterAsync()
    {
        co_await Context()->NetworkRunner().OnConnectedAsync();
        
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        playerAvatar_ = Context()->NetworkRunner().SpawnPlayerAvatar(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            glm::quat());
    }

    void GrassLandScene::Enter()
    {
        
    }

    void GrassLandScene::DoDispose()
    {
        PlayerAvatar::SaveType(*playerAvatar_.lock());
        playerAvatar_.lock()->SaveStatus();
        
        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
    }

    void GrassLandScene::OnDrawGui()
    {
        
    }
}
