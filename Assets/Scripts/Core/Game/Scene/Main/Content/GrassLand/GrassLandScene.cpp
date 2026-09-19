#include "GrassLandScene.h"

#include "../../../../../../GamePlay/Network/Session/GamePlay_StageSessionMatchmaking.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

#include "DxLib.h"
#include "../../Loading/Main_SceneLoadStep.h"

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
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

        ++loadGeneration_;
        Coroutine::StartCoroutine(OnEnterAsync(loadGeneration_));
    }

    Coroutine::Task<void> GrassLandScene::OnEnterAsync(const int generation)
    {
        LoadingScreen().SetStep(SceneLoadStep::Deserializing);

        co_await LoadMainSceneAsync();
        if (generation != loadGeneration_)
            co_return;

        if (HasMainSceneLoadFailed())
        {
            LoadingScreen().Fail("ステージの読み込みに失敗しました");
            Coroutine::StartCoroutine(BackToMainIslandAsync(generation));
            co_return;
        }

        scene_ = LoadedMainScene();

        // Context の FIELD は読み込んだシーン内の GameObject を指すので、
        // AddContent による解決が済むこのタイミングより前には触れない
        LoadingScreen().SetStep(SceneLoadStep::Warmup);
        Context()->Init();

        // メインシーンが居ない間に Instantiate が走らないよう、ロード完了まで待ってから積む
        SubScene().Push(Sub::SceneType::ChattingUI);
        SubScene().Push(Sub::SceneType::OtherPlayerStatus);

        LoadingScreen().SetStep(SceneLoadStep::Connecting);
        co_await GamePlay::Network::JoinOrHostStageAsync(Context()->WeakNetworkRunner(), std::string(ToString(SceneType::GrassLand)));
        if (generation != loadGeneration_)
            co_return;

        auto& networkRunner = Context()->NetworkRunner();
        if (!networkRunner.IsStarted() || networkRunner.GetConnectionState() != Core::Network::ConnectionState::Connected)
        {
            LoadingScreen().Fail("マルチプレイの接続に失敗しました");
            Coroutine::StartCoroutine(BackToMainIslandAsync(generation));
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

        LoadingScreen().SetStep(SceneLoadStep::Completed);
        LoadingScreen().BeginHide();

        Coroutine::StartCoroutine(GrassLand::GrassLandArrivalMovie::PlayAsync(arrivalMovie_));
    }

    Coroutine::Task<void> GrassLandScene::BackToMainIslandAsync(const int generation)
    {
        const int startedMs = GetNowCount();
        co_await Coroutine::WaitUntil([startedMs] { return GetNowCount() - startedMs >= 2000; });
        if (generation != loadGeneration_)
            co_return;

        Game::Instance().Scenes().RequestChangeScene(SceneType::MainIsland);
        LoadingScreen().BeginHide();
    }

    void GrassLandScene::Enter()
    {

    }

    void GrassLandScene::DoDispose()
    {
        // 走っているロードコルーチンを無効化する。コルーチン自体は止められない
        ++loadGeneration_;

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
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
        scene_.reset();
    }

    void GrassLandScene::OnDrawGui()
    {

    }
}
