#include "MainIsLandScene.h"

#include "../../../../PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"

namespace GameCore::Scene::Main
{
    MainIslandScene::MainIslandScene(
        const std::weak_ptr<MainIslandSceneContext>& context,
        GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, baseContext)
    {
        
    }

    MainIslandScene::~MainIslandScene() = default;

    void MainIslandScene::Init()
    {
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        scene_ = LoadMainScene();
        Context()->Init();
        
        // PlayerAvatarの生成
        playerAvatar_ = PlayerAvatar::Factory::SummonSwordManAvatar(
              Context()->SummonPlayerAvatarPrefab()
            , Context()->PlayerSpawnPoint()
            , nullptr);
    }

    void MainIslandScene::OnEnter()
    {
        
    }

    void MainIslandScene::OnExit()
    {
        
    }

    void MainIslandScene::Dispose()
    {
        
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
