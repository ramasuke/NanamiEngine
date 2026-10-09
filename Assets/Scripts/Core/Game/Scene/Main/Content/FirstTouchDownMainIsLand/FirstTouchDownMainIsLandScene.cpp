#include "FirstTouchDownMainIsLandScene.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../PlayerAvatar/Status/NullPlayerAvatarStatus.h"
#include "../../../../Story/Story_StoryProgress.h"
#include "../../../../Game.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"
#include "../../Group/Main_GameSceneGroup.h"
#include "AboardAirShipMovie/AboardAirShipMovie.h"

namespace GameCore::Scene::Main
{
    FirstTouchDownMainIsLandScene::FirstTouchDownMainIsLandScene(
        const std::weak_ptr<FirstTouchDownMainIsLandSceneContext>& context,
        const GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, baseContext)
    {
        
    }

    FirstTouchDownMainIsLandScene::~FirstTouchDownMainIsLandScene() = default;

    std::vector<Sub::SceneType> FirstTouchDownMainIsLandScene::SubScenes() const
    {
        return { Sub::SceneType::ChattingUI };
    }

    void FirstTouchDownMainIsLandScene::OnInit()
    {
        // NOTE: OnEnterAsync は読み込みの後に走るので、遷移のオプションはここで取っておく
        isRetry_ = Game::Instance().Scenes().TransitionOptions().isRetry;
    }

    Coroutine::Task<EnterResult> FirstTouchDownMainIsLandScene::OnEnterAsync(NanamiEngine::R4::CancellationToken)
    {
        Context()->Init();

        auto& context = *Context();

        GamePlay::Sound::SoundPlayer::PlayBgm(context.BGM());

        // NOTE: スポーン位置は船の子なので、主人公を出す前に着岸させる
        if (isRetry_)
            FirstTouchDownMainIsLand::AboardAirShipMovie::DockImmediately(context);

        playerAvatar_ = context.PlayerAvatarFactory().LoadInitedPlayerAvatar(
            PlayerAvatar::PlayerAvatarType::SwordMan,
            context.PlayerSpawnPoint(),
            isRetry_ ? nullptr : context.AirShip()->Entity().lock(),
            true,
            std::make_shared<PlayerAvatar::NullPlayerAvatarStatus>());

        if (isRetry_)
        {
            if (const auto avatar = playerAvatar_.lock())
                avatar->PlayerTransform().LookAtY(context.PlayerFirstMoveTarget().GetWorldPos());
            co_return EnterResult::Ok();
        }

        aboardAirShipMovie_ = std::make_shared<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        Coroutine::StartCoroutine(FirstTouchDownMainIsLand::AboardAirShipMovie::PlayAsync(aboardAirShipMovie_));
        co_return EnterResult::Ok();
    }

    void FirstTouchDownMainIsLandScene::Enter()
    {

    }

    void FirstTouchDownMainIsLandScene::DoExit()
    {
        if (aboardAirShipMovie_)
            aboardAirShipMovie_->Cancel();
        
        aboardAirShipMovie_.reset();

        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SelectedPlayerAvatarType::Save(*avatar);
            avatar->PlayerStatus().RestoreFullHealth();
            avatar->SaveStatus();

            // NOTE: 死んでやり直す・タイトルへ戻るときもここを通るので、序章を終えたときだけ進める
            if (Game::Instance().Scenes().TransitionOptions().isStageCleared)
            {
                SaveGameProgression(GameProgresion::MainIsland);
                Story::StoryProgress::Instance().Set(Story::StoryFlag::PrologueCleared);
            }
        }
        playerAvatar_.reset();

        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }
    
    void FirstTouchDownMainIsLandScene::OnDrawGui()
    {
        
    }
}
