#include "MainIsLandScene.h"

#include "Engine/Module/GameObject/Transform/Transform.h"

#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../PlayerAvatar/Status/NullPlayerAvatarStatus.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"
#include "../../../../Story/Story_StoryProgress.h"
#include "../../../../Game.h"
#include "../../Group/Main_GameSceneGroup.h"
#include "../../../../../../GamePlay/Prop/FloatingStone/Prop_StoryMovieParts.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"

namespace GameCore::Scene::Main
{
    MainIslandScene::MainIslandScene(
        const std::weak_ptr<MainIslandSceneContext>& context,
        GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, baseContext)
    {
        
    }

    MainIslandScene::~MainIslandScene() = default;

    std::vector<Sub::SceneType> MainIslandScene::SubScenes() const
    {
        return { Sub::SceneType::ChattingUI };
    }

    Coroutine::Task<EnterResult> MainIslandScene::OnEnterAsync(NanamiEngine::R4::CancellationToken)
    {
        // NOTE: Context の FIELD は読み込んだシーン内を指すので、読み込みが済んだここで初めて触る
        Context()->Init();

        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            PlayerAvatar::SelectedPlayerAvatarType::Load(),
            Context()->PlayerSpawnPoint(),
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;

        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        isDeparting_ = false;
        ApplyStageRewards();
        co_return EnterResult::Ok();
    }

    namespace
    {
        Coroutine::Task<void> PlayStageRewardsAsync(
            std::weak_ptr<GamePlay::Prop::FloatingStone> weakGreenStone,
            std::weak_ptr<GamePlay::Prop::ReturningIsland> weakIsland,
            std::weak_ptr<GamePlay::Prop::FloatingStone> weakLightStone,
            std::weak_ptr<IPlayerAvatar> playerAvatar,
            std::function<bool()> canStart)
        {
            const auto greenStone = weakGreenStone.lock();
            const auto island     = weakIsland.lock();
            const auto lightStone = weakLightStone.lock();

            if (greenStone)
            {
                co_await greenStone->PlayReturnAsync(
                    playerAvatar, canStart,
                    [] { Story::StoryProgress::Instance().Set(Story::StoryFlag::GreenStoneReturned); });
                if (greenStone->DestroyCancellationToken().IsCancellationRequested())
                    co_return;
            }
            if (island)
            {
                co_await island->PlayReturnAsync(
                    playerAvatar, canStart,
                    [] { Story::StoryProgress::Instance().Set(Story::StoryFlag::FountainIslandReturned); });
                if (island->DestroyCancellationToken().IsCancellationRequested())
                    co_return;
            }
            if (lightStone)
            {
                co_await lightStone->PlayReturnAsync(
                    playerAvatar, canStart,
                    [] { Story::StoryProgress::Instance().Set(Story::StoryFlag::LightStoneReturned); });
            }
        }

        Coroutine::Task<void> PlayNestDepartureAsync(
            std::weak_ptr<CineMachine::CineMachineVirtualCamera> weakCamera,
            std::shared_ptr<Asset::SoundFile> rumble,
            std::weak_ptr<IPlayerAvatar> playerAvatar,
            const float departure_secs)
        {
            // NOTE: カメラはシーンの物なのでローカルに持ち直す
            const auto camera = weakCamera.lock();
            GamePlay::Prop::StoryMovie::CameraScope scope(playerAvatar, camera, nullptr, glm::vec3(0.0f));
            scope.Begin();
            if (rumble)
                rumble->Play();

            float elapsed_secs = 0.0f;
            while (elapsed_secs < departure_secs)
            {
                co_await Coroutine::WaitYield();
                if (camera && camera->DestroyCancellationToken().IsCancellationRequested())
                    co_return;
                elapsed_secs += Time::DeltaTime();
                // NOTE: 島が引きずられはじめ、だんだん揺れが強くなる
                const float rate = GamePlay::Prop::StoryMovie::Rate(elapsed_secs, departure_secs);
                NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::SustainShakeMainCamera(0.15f + 0.55f * rate);
            }

            Game::Instance().Scenes().RequestChangeScene(SceneType::DragonNest);
            // NOTE: ロード画面が覆い切るまで演出のカメラのままにする (シーンが破棄されるとカメラも消える)
            while (camera && !camera->DestroyCancellationToken().IsCancellationRequested())
                co_await Coroutine::WaitYield();
        }
    }

    void MainIslandScene::ApplyStageRewards()
    {
        const auto& story = Story::StoryProgress::Instance();
        const auto island = Context()->FountainIsland();
        const bool isIslandReturned = story.IsSet(Story::StoryFlag::FountainIslandReturned);
        if (island)
        {
            if (isIslandReturned)
                island->Show();
            else
                island->Sink();
        }

        const auto greenStone = Context()->GreenStone();
        const bool isGrassLandCleared = story.IsSet(Story::StoryFlag::GrassLandCleared);
        if (greenStone && !isGrassLandCleared)
            greenStone->SetVisible(false);

        const auto lightStone = Context()->LightStone();
        const bool isDesertCleared = story.IsSet(Story::StoryFlag::DesertCleared);
        if (lightStone && !isDesertCleared)
            lightStone->SetVisible(false);

        const bool playsGreen  = greenStone && isGrassLandCleared && !story.IsSet(Story::StoryFlag::GreenStoneReturned);
        const bool playsIsland = island && isGrassLandCleared && !isIslandReturned;
        const bool playsLight  = lightStone && isDesertCleared && !story.IsSet(Story::StoryFlag::LightStoneReturned);
        if (!playsGreen && !playsIsland && !playsLight)
            return;

        // NOTE: 演出が石を出す。飛んでくるまでは島の底に見えないよう先に隠す
        if (playsGreen)
            greenStone->SetVisible(false);
        if (playsLight)
            lightStone->SetVisible(false);
        Coroutine::StartCoroutine(PlayStageRewardsAsync(
            playsGreen  ? greenStone : nullptr,
            playsIsland ? island     : nullptr,
            playsLight  ? lightStone : nullptr,
            playerAvatar_,
            // NOTE: 呼ばれるのはシーンが残っている間だけ(抜けたら石と島が破棄され、演出が先に止まる)
            [this] { return !LoadingScreen().IsShown(); }));
    }

    void MainIslandScene::BeginNestDeparture()
    {
        if (isDeparting_ || Game::Instance().Scenes().HasPendingChange())
            return;
        isDeparting_ = true;

        Coroutine::StartCoroutine(PlayNestDepartureAsync(
            Context()->NestDepartureCamera(),
            Context()->NestDepartureSound(),
            playerAvatar_,
            Context()->NestDeparture_secs()));
    }

    void MainIslandScene::SwitchPlayerAvatar(const PlayerAvatar::PlayerAvatarType type)
    {
        const auto current = playerAvatar_.lock();
        if (!current || current->Type() == type)
            return;

        const auto position = current->PlayerTransform().GetWorldPos();
        current->SaveStatus();

        Context()->PlayerAvatarFactory().DestroyAttachments(attachments_);
        current->PlayerTransform().GetGameObject()->OnDestroy();
        playerAvatar_.reset();
        attachments_ = {};

        PlayerAvatar::SelectedPlayerAvatarType::Save(type);

        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            type,
            position,
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;
    }

    void MainIslandScene::Enter()
    {
        
    }

    void MainIslandScene::DoExit()
    {
        // NOTE: 読み込みの途中で抜けたときはアバターが居ない。そのときは進行も保存しない
        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SelectedPlayerAvatarType::Save(*avatar);
            avatar->SaveStatus();
            SaveGameProgression(GameProgresion::MainIsland);
        }
        playerAvatar_.reset();
        attachments_ = {};
        
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
