#include "GrassLandScene.h"

#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>
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

        playerAvatar_ = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatar(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            nullptr,
            Context()->CameraGroup());
    }

    void GrassLandScene::Enter()
    {
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
    }

    void GrassLandScene::Dispose()
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
