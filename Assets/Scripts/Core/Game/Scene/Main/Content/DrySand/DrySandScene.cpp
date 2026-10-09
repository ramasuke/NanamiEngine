#include "DrySandScene.h"

#include "../../../../../../GamePlay/Network/Session/GamePlay_StageSessionMatchmaking.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

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
#include "../GrassLand/ArrivalMovie/GrassLandArrivalMovie.h"
#include "../../../../PlayerAvatar/Record/PlayerAvatar_RecordBook.h"
#include "../../../../Story/Story_StageClear.h"
#include "../../../../Story/Story_StoryProgress.h"

namespace GameCore::Scene::Main
{
    DrySandScene::DrySandScene(
        const std::weak_ptr<DrySandSceneContext>& context,
        GameSceneBaseContext baseContext)
            : GameMainSceneBase(context, baseContext)
    {
    }

    DrySandScene::~DrySandScene() = default;

    void DrySandScene::OnInit()
    {
        if (!Context())
        {
            throw std::runtime_error("DrySandSceneContextが設定されていません。GameManage.sceneにDrySandSceneContextを追加してください。");
        }

        // NOTE: 記録帳と同じく協力プレイでも各ピアで立つ(物語の進み具合は共有しない)
        if (const auto stageClear = Context()->StageClear())
        {
            stageClearWatcher_.Watch(
                PlayerAvatar::Record::RecordBook::Instance().OnDefeat(),
                *stageClear,
                [this](const Story::StoryFlag flag) { OnStageClear(flag); });
        }
    }

    std::vector<Sub::SceneType> DrySandScene::SubScenes() const
    {
        return { Sub::SceneType::ChattingUI, Sub::SceneType::OtherPlayerStatus, Sub::SceneType::StageReturn };
    }

    Coroutine::Task<EnterResult> DrySandScene::OnEnterAsync(const NanamiEngine::R4::CancellationToken token)
    {
        // WARNING: Context の FIELD は読み込んだシーン内の GameObject を指す。読み込みが済むまで触らない
        Context()->Init();

        // NOTE: 浮遊石はもう拠点の島へ飛び去っている
        if (const auto stone = Context()->FloatingStone(); stone && Story::StoryProgress::Instance().IsSet(Story::StoryFlag::DesertCleared))
            stone->SetVisible(false);

        LoadingScreen().SetStep(SceneLoadStep::Connecting);
        const auto joinFailure = co_await GameCore::Game::Instance().Matchmaker().JoinOrHostAsync(
            Context()->WeakNetworkRunner(), std::string(ToString(SceneType::Desert)));
        if (token.IsCancellationRequested())
            co_return {};

        auto& networkRunner = Context()->NetworkRunner();
        if (!networkRunner.IsStarted() || networkRunner.GetConnectionState() != Core::Network::ConnectionState::Connected)
        {
            co_return EnterResult::Fail(joinFailure.value_or("マルチプレイの接続に失敗しました"));
        }

        LoadingScreen().SetStep(SceneLoadStep::Spawning);
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        playerAvatar_ = networkRunner.SpawnPlayerAvatar(
            PlayerAvatar::SelectedPlayerAvatarType::Load(),
            Context()->PlayerSpawnPoint(),
            Context()->PlayerSpawnRotation());

        // NOTE: 敵はホストだけがスポーンする
        if (networkRunner.IsServer())
        {
            for (const auto& spawnPoint : Context()->EnemySpawnPoints())
            {
                networkRunner.SpawnEnemy(
                    spawnPoint->Kind(),
                    spawnPoint->Prefab(),
                    spawnPoint->Transform().GetWorldPos(),
                    spawnPoint->Transform().GetWorldRot());
            }
        }

        // NOTE: カバーが明ける前に画を作っておく
        arrivalMovie_ = std::make_shared<GrassLand::StageArrivalMovie<DrySandSceneContext>>(
            playerAvatar_, Context(), Story::StoryFlag::DesertOverviewSeen);
        arrivalMovie_->Begin();
        co_return EnterResult::Ok();
    }

    void DrySandScene::OnEntered()
    {
        Coroutine::StartCoroutine(GrassLand::StageArrivalMovie<DrySandSceneContext>::PlayAsync(arrivalMovie_));
    }

    void DrySandScene::Enter()
    {

    }

    void DrySandScene::OnStageClear(const Story::StoryFlag flag)
    {
        isStageCleared_ = true;

        // NOTE: 初めて立てたときだけ。倒し直しでは石はもう無い
        if (!Story::StoryProgress::Instance().Set(flag) || !Context())
            return;

        if (const auto stone = Context()->FloatingStone())
            Coroutine::StartCoroutine(stone->PlayDepartAsync(playerAvatar_));
    }

    void DrySandScene::DoExit()
    {
        stageClearWatcher_.Dispose();

        if (arrivalMovie_)
            arrivalMovie_->Cancel();
        arrivalMovie_.reset();

        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SelectedPlayerAvatarType::Save(*avatar);
            if (isStageCleared_)
                avatar->PlayerStatus().RestoreFullHealth();
            avatar->SaveStatus();
        }
        playerAvatar_.reset();

        // NOTE: ボスの BT (PlayBGM) が差し替えた BGM も流れているので、シーンの BGM だけでなく全部止める
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }

    void DrySandScene::OnDrawGui()
    {

    }
}
