#include "GrassLandScene.h"

#include "../../../../../../GamePlay/Network/Session/GamePlay_StageSessionMatchmaking.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

#include "DxLib.h"
#include "../../Loading/Main_SceneLoadStep.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../Group/Main_GameSceneGroup.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"
#include "../../../../Game.h"
#include "ArrivalMovie/GrassLandArrivalMovie.h"

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

        Coroutine::StartCoroutine(OnEnterAsync(BeginEnter()));
    }

    Coroutine::Task<void> GrassLandScene::OnEnterAsync(const int generation)
    {
        if (!co_await LoadMainSceneAsync(generation))
            co_return;

        // Context の FIELD は読み込んだシーン内の GameObject を指すので、
        // AddContent による解決が済むこのタイミングより前には触れない
        Context()->Init();

        // メインシーンが居ない間に Instantiate が走らないよう、ロード完了まで待ってから積む
        SubScene().Push(Sub::SceneType::ChattingUI);
        SubScene().Push(Sub::SceneType::OtherPlayerStatus);

        LoadingScreen().SetStep(SceneLoadStep::Connecting);
        co_await GamePlay::Network::JoinOrHostStageAsync(Context()->WeakNetworkRunner(), std::string(ToString(SceneType::GrassLand)));
        if (!IsCurrentEnter(generation))
            co_return;

        auto& networkRunner = Context()->NetworkRunner();
        if (!networkRunner.IsStarted() || networkRunner.GetConnectionState() != Core::Network::ConnectionState::Connected)
        {
            FailEnter(generation, "マルチプレイの接続に失敗しました");
            co_return;
        }

        LoadingScreen().SetStep(SceneLoadStep::Spawning);
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        playerAvatar_ = networkRunner.SpawnPlayerAvatar(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            glm::quat());

        // 敵はホスト側だけがスポーンする。クライアント側は
        // EnemySpawnDispatcher::OnReceive(ライブ受信 or 再接続時の履歴リプレイ)で再現される。
        if (networkRunner.IsServer())
        {
            for (const auto& spawnPoint : Context()->EnemySpawnPoints())
            {
                networkRunner.SpawnEnemy(
                    spawnPoint->Kind(),
                    spawnPoint->Transform().GetWorldPos(),
                    spawnPoint->Transform().GetWorldRot());
            }
        }

        // カバーが明ける前に画を作っておく
        arrivalMovie_ = std::make_shared<GrassLand::GrassLandArrivalMovie>(playerAvatar_, Context());
        arrivalMovie_->Begin();

        CompleteEnter(generation);

        Coroutine::StartCoroutine(GrassLand::GrassLandArrivalMovie::PlayAsync(arrivalMovie_));
    }

    void GrassLandScene::Enter()
    {

    }

    void GrassLandScene::DoDispose()
    {
        if (arrivalMovie_)
            arrivalMovie_->Cancel();
        arrivalMovie_.reset();

        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SaveType(*avatar);
            avatar->SaveStatus();
        }
        playerAvatar_.reset();

        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
    }

    void GrassLandScene::OnDrawGui()
    {

    }
}
