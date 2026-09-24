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
        Coroutine::StartCoroutine(OnEnterAsync(BeginEnter()));
    }

    Coroutine::Task<void> MainIslandScene::OnEnterAsync(const int generation)
    {
        if (!co_await LoadMainSceneAsync(generation))
            co_return;

        // Context の FIELD は読み込んだシーン内を指すので、AddContent が済んだここで初めて触る
        Context()->Init();
        // メインシーンが居ない間に Instantiate が走らないよう、読み込みが済んでから積む
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;

        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        ApplyGrassLandReward();
        CompleteEnter(generation);
    }

    namespace
    {
        Coroutine::Task<void> PlayGrassLandRewardAsync(
            std::shared_ptr<GamePlay::Prop::FloatingStone> stone,
            std::shared_ptr<GamePlay::Prop::ReturningIsland> island,
            std::weak_ptr<IPlayerAvatar> playerAvatar,
            std::function<bool()> canStart)
        {
            if (stone)
            {
                co_await stone->PlayReturnAsync(
                    playerAvatar, canStart,
                    [] { Story::StoryProgress::Instance().Set(Story::StoryFlag::GreenStoneReturned); });
                // NOTE: シーンを抜けたら島は次に来たときに改めて戻す
                if (stone->DestroyCancellationToken().IsCancellationRequested())
                    co_return;
            }
            if (!island)
                co_return;

            co_await island->PlayReturnAsync(
                playerAvatar, canStart,
                [] { Story::StoryProgress::Instance().Set(Story::StoryFlag::FountainIslandReturned); });
        }
    }

    void MainIslandScene::ApplyGrassLandReward()
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

        const auto stone = Context()->GreenStone();
        if (!story.IsSet(Story::StoryFlag::GrassLandCleared))
        {
            if (stone)
                stone->SetVisible(false);
            return;
        }

        const bool playsStone  = stone && !story.IsSet(Story::StoryFlag::GreenStoneReturned);
        const bool playsIsland = island && !isIslandReturned;
        if (!playsStone && !playsIsland)
            return;

        // NOTE: 演出が石を出す。飛んでくるまでは島の底に見えないよう先に隠す
        if (playsStone)
            stone->SetVisible(false);
        Coroutine::StartCoroutine(PlayGrassLandRewardAsync(
            playsStone  ? stone  : nullptr,
            playsIsland ? island : nullptr,
            playerAvatar_,
            // NOTE: 呼ばれるのはシーンが残っている間だけ(抜けたら石と島が破棄され、演出が先に止まる)
            [this] { return !LoadingScreen().IsShown(); }));
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

        PlayerAvatar::SaveType(type);

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

    void MainIslandScene::DoDispose()
    {
        // 読み込みの途中で抜けたときはアバターが居ない。そのときは進行も保存しない
        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SaveType(*avatar);
            avatar->SaveStatus();
            SaveGameProgression(GameProgresion::GrassLandStage);
        }
        playerAvatar_.reset();
        attachments_ = {};
        
        // NOTE: 敵の BT (PlayBGM) が差し替えた BGM も残さないよう、シーンの BGM だけでなく全部止める
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
