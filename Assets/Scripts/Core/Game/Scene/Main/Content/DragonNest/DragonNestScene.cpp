#include "DragonNestScene.h"

#include "../../../../../../GamePlay/Network/Session/GamePlay_StageSessionMatchmaking.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include <stdexcept>

#include "../../Loading/Main_SceneLoadStep.h"

#include "Engine/Core/Coroutine/Coroutine.h"
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
#include "../../../../../../GamePlay/Prop/FloatingDrift/GamePlay_FloatingDrift.h"
#include "../../../../../../GamePlay/Prop/FloatingStone/Prop_StoryMovieParts.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include <cmath>

namespace GameCore::Scene::Main
{
    DragonNestScene::DragonNestScene(
        const std::weak_ptr<DragonNestSceneContext>& context,
        GameSceneBaseContext baseContext)
            : GameMainSceneBase(context, baseContext)
    {
    }

    DragonNestScene::~DragonNestScene() = default;

    void DragonNestScene::OnInit()
    {
        if (!Context())
        {
            throw std::runtime_error("DragonNestSceneContextが設定されていません。GameManage.sceneにDragonNestSceneContextを追加してください。");
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

    std::vector<Sub::SceneType> DragonNestScene::SubScenes() const
    {
        return { Sub::SceneType::ChattingUI, Sub::SceneType::OtherPlayerStatus, Sub::SceneType::StageReturn };
    }

    Coroutine::Task<EnterResult> DragonNestScene::OnEnterAsync(const NanamiEngine::R4::CancellationToken token)
    {
        // Context の FIELD は読み込んだシーン内の GameObject を指すので、読み込みが済んだここで初めて触る
        Context()->Init();

        LoadingScreen().SetStep(SceneLoadStep::Connecting);
        const auto joinFailure = co_await GameCore::Game::Instance().Matchmaker().JoinOrHostAsync(
            Context()->WeakNetworkRunner(), std::string(ToString(SceneType::DragonNest)));
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
            glm::quat());

        // 敵はホスト側だけがスポーンする。クライアント側は
        // EnemySpawnDispatcher::OnReceive(ライブ受信 or 再接続時の履歴リプレイ)で再現される。
        if (networkRunner.IsServer())
        {
            for (const auto& spawnPoint : Context()->EnemySpawnPoints())
            {
                if (!spawnPoint->ShouldSpawn())
                    continue;
                networkRunner.SpawnEnemy(
                    spawnPoint->Kind(),
                    spawnPoint->Transform().GetWorldPos(),
                    spawnPoint->Transform().GetWorldRot());
            }
        }

        // カバーが明ける前に画を作っておく
        arrivalMovie_ = std::make_shared<GrassLand::StageArrivalMovie<DragonNestSceneContext>>(playerAvatar_, Context());
        arrivalMovie_->Begin();
        co_return EnterResult::Ok();
    }

    void DragonNestScene::OnEntered()
    {
        Coroutine::StartCoroutine(GrassLand::StageArrivalMovie<DragonNestSceneContext>::PlayAsync(arrivalMovie_));
    }

    void DragonNestScene::Enter()
    {

    }

    namespace
    {
        using GameObjectPtr = std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>;
        namespace Movie = GamePlay::Prop::StoryMovie;

        /** @brief 散っていく心臓1つ。浮き上がってから、それぞれの向きへ空の彼方へ飛んでいく */
        struct FlyingHeart
        {
            GameObjectPtr heart;
            std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile> trailPrefab;
            glm::vec3 basePos;
            glm::quat baseRot;
            glm::vec3 direction;
            float delay_secs  = 0.0f;
            float spinDegrees = 0.0f;
            std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> trail;
            bool isLaunched = false;
            bool isGone     = false;
        };

        bool IsCanceled(const std::shared_ptr<CineMachine::CineMachineVirtualCamera>& camera)
        {
            return camera && camera->DestroyCancellationToken().IsCancellationRequested();
        }

        /**
         * @brief 古竜が倒れた後、巣に積まれていた心臓が空へ散り散りに飛んでいく (docs/Story.md 終章の結末)。
         *        終わったら拠点の島へ戻る。カメラが破棄されたら (シーンを抜けたら) 止まる
         */
        Coroutine::Task<void> PlayHeartScatterAsync(
            std::shared_ptr<DragonNestSceneContext> context, std::weak_ptr<IPlayerAvatar> playerAvatar)
        {
            const auto camera = context->EndingCamera();

            float delay_secs = 0.0f;
            while (delay_secs < context->EndingDelay_secs())
            {
                co_await Coroutine::WaitYield();
                if (IsCanceled(camera))
                    co_return;
                delay_secs += Time::DeltaTime();
            }

            const glm::vec3 center = context->HeartMoundCenter();
            const float riseHeight = context->HeartRiseHeight();
            std::vector<FlyingHeart> hearts;
            const auto targets = context->ScatterHearts();
            const float count = static_cast<float>(std::max<size_t>(targets.size(), 1));
            for (size_t i = 0; i < targets.size(); ++i)
            {
                const auto& heart = targets[i];
                // NOTE: 漂う心臓は FloatingDrift が毎フレーム位置を決めるので止める
                if (const auto drift = heart->Components().Catch<GamePlay::Prop::FloatingDrift>().lock())
                    drift->SetEnable(false);

                // 黄金角で散らして、どの方角の空にも心臓が飛んでいくようにする
                const float index = static_cast<float>(i);
                const float angle = glm::radians(137.5f * index);
                const float up    = 0.55f + 0.35f * std::fmod(index * 0.618f, 1.0f);
                FlyingHeart flying;
                flying.heart       = heart;
                flying.trailPrefab = context->HeartTrail(heart->Name());
                flying.basePos     = heart->Transform().GetWorldPos();
                flying.baseRot     = heart->Transform().GetWorldRot();
                flying.direction   = glm::normalize(glm::vec3(std::cos(angle), up, std::sin(angle)));
                flying.delay_secs  = context->HeartStagger_secs() * index / count;
                flying.spinDegrees = 360.0f + 180.0f * std::fmod(index * 0.37f, 1.0f);
                hearts.push_back(std::move(flying));
            }

            // NOTE: LookAt の的は心臓の1つにして、ずらしで山の上 (心臓が浮き上がる高さ) を見る
            GameObjectPtr lookTarget = hearts.empty() ? nullptr : hearts.front().heart;
            const glm::vec3 lookOffset = hearts.empty() ? glm::vec3(0.0f) : center + glm::vec3(0.0f, riseHeight, 0.0f) - hearts.front().basePos;
            Movie::CameraScope scope(playerAvatar, camera, lookTarget, lookOffset);
            scope.Begin();

            // 心臓の山が割れるように光って、心臓が次々に浮き上がる
            if (const auto burst = context->HeartBurst())
                NanamiEngine::Scene::GameObject::Instantiate(burst, center);
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(0.8f, 2.5f);

            const float rise_secs = context->HeartRise_secs();
            const float fly_secs  = context->HeartFly_secs();
            const float end_secs  = context->HeartStagger_secs() + rise_secs + fly_secs + context->EndingHold_secs();
            Movie::SkipInput skip;
            float elapsed_secs = 0.0f;
            bool isCanceled = false;
            while (elapsed_secs < end_secs)
            {
                co_await Coroutine::WaitYield();
                if (IsCanceled(camera))
                {
                    isCanceled = true;
                    break;
                }
                elapsed_secs += Time::DeltaTime();
                // NOTE: 倒した直後は攻撃ボタンを連打しているので、しばらくはスキップを受け付けない
                if (skip.IsSkipped() && elapsed_secs >= 2.0f)
                    break;

                for (auto& flying : hearts)
                {
                    const float local_secs = elapsed_secs - flying.delay_secs;
                    if (flying.isGone || local_secs < 0.0f)
                        continue;

                    auto& transform = flying.heart->Transform();
                    const glm::vec3 risenPos = flying.basePos + glm::vec3(0.0f, riseHeight, 0.0f);
                    if (local_secs < rise_secs)
                    {
                        const float t = Movie::Rate(local_secs, rise_secs);
                        transform.SetWorldPos(glm::mix(flying.basePos, risenPos, Movie::EaseOutCubic(t)));
                        transform.SetWorldRot(Movie::Yaw(flying.spinDegrees * 0.25f * Movie::EaseInOutSine(t)) * flying.baseRot);
                        continue;
                    }

                    if (!flying.isLaunched)
                    {
                        flying.isLaunched = true;
                        if (flying.trailPrefab)
                            flying.trail = NanamiEngine::Scene::GameObject::Instantiate(flying.trailPrefab, risenPos);
                    }

                    const float t = Movie::Rate(local_secs - rise_secs, fly_secs);
                    const glm::vec3 pos = risenPos + flying.direction * context->HeartFlyDistance() * Movie::EaseInCubic(t);
                    transform.SetWorldPos(pos);
                    transform.SetWorldRot(Movie::Yaw(flying.spinDegrees * (0.25f + Movie::EaseInCubic(t))) * flying.baseRot);
                    if (const auto trail = flying.trail.lock())
                        trail->Transform().SetWorldPos(pos);

                    if (t >= 1.0f)
                    {
                        flying.isGone = true;
                        flying.heart->SetEnable(false);
                        if (const auto trail = flying.trail.lock())
                            trail->OnDestroy();
                    }
                }
            }

            for (auto& flying : hearts)
            {
                if (const auto trail = flying.trail.lock())
                    trail->OnDestroy();
            }
            if (isCanceled)
                co_return;

            // NOTE: 古竜を倒した記録はもう付いている。地図に踏破の印を押して拠点の島へ帰る
            Game::Instance().Scenes().RequestChangeScene(
                SceneType::MainIsland, SceneTransitionOptions{ .isStageCleared = true });
            // NOTE: ロード画面が覆い切るまで、心臓の山を映したままにする
            while (camera && !IsCanceled(camera))
                co_await Coroutine::WaitYield();
        }
    }

    void DragonNestScene::OnStageClear(const Story::StoryFlag flag)
    {
        isStageCleared_ = true;

        // 初めて立てたときだけ。倒し直しでは心臓はもう散っている
        if (!Story::StoryProgress::Instance().Set(flag) || !Context())
            return;

        Coroutine::StartCoroutine(PlayHeartScatterAsync(Context(), playerAvatar_));
    }

    void DragonNestScene::DoExit()
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

    void DragonNestScene::OnDrawGui()
    {

    }
}
