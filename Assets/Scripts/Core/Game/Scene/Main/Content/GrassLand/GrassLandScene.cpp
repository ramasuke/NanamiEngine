#include "GrassLandScene.h"

#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../Engine/Core/Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
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
        SubScene().Push(Sub::SceneType::OtherPlayerStatus);
        
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

        // 敵はホスト(NetworkMode::Server)側だけがスポーンする。クライアント側は
        // EnemySpawnDispatcher::OnReceive(ライブ受信 or 再接続時の履歴リプレイ)で再現される。
        if (Core::Application::Configuration::NetworkConfiguration::IsServer())
        {
            for (const auto& spawnPoint : Context()->EnemySpawnPoints())
            {
                Context()->NetworkRunner().SpawnEnemy(
                    Context()->EnemyPrefab(),
                    spawnPoint->Transform().GetWorldPos(),
                    spawnPoint->Transform().GetWorldRot());
            }
        }
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
