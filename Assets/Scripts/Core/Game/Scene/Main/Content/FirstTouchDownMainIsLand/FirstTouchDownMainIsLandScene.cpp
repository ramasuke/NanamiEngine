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
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"
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

    void FirstTouchDownMainIsLandScene::Init()
    {
        Coroutine::StartCoroutine(OnEnterAsync(BeginEnter()));
    }

    Coroutine::Task<void> FirstTouchDownMainIsLandScene::OnEnterAsync(const int generation)
    {
        if (!co_await LoadMainSceneAsync(generation))
            co_return;

        // Context の FIELD(飛行船・カメラ・タイトルロゴ)は読み込んだシーン内を指す
        Context()->Init();
        // メインシーンが居ない間に Instantiate が走らないよう、読み込みが済んでから積む
        SubScene().Push(Sub::SceneType::ChattingUI);

        auto& context = *Context();
        
        GamePlay::Sound::SoundPlayer::PlayBgm(context.BGM());
        
        /** Player生成処理 */
        playerAvatar_ = context.PlayerAvatarFactory().LoadInitedPlayerAvatar(
            PlayerAvatar::PlayerAvatarType::SwordMan,
            context.PlayerSpawnPoint(),
            context.AirShip()->Entity().lock(),
            true,
            std::make_shared<PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_.lock()->PlayerTransform().SetLocalRot({glm::vec3{0.0f, 90.0f, 0.0f}});
        
        // 船を降りるまでのMovie開始
        aboardAirShipMovie_ = std::make_shared<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        Coroutine::StartCoroutine(FirstTouchDownMainIsLand::AboardAirShipMovie::PlayAsync(aboardAirShipMovie_));

        CompleteEnter(generation);
    }

    void FirstTouchDownMainIsLandScene::Enter()
    {

    }

    void FirstTouchDownMainIsLandScene::DoDispose()
    {
        // ムービーのコルーチンは止められないので、次の区切りで抜けさせる
        if (aboardAirShipMovie_)
            aboardAirShipMovie_->Cancel();
        aboardAirShipMovie_.reset();

        // 読み込みの途中で抜けたときはアバターが居ない。そのときは進行も保存しない
        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SaveType(*avatar);
            avatar->SaveStatus();
            SaveGameProgression(GameProgresion::MainIsland);
            Story::StoryProgress::Instance().Set(Story::StoryFlag::PrologueCleared);
        }
        playerAvatar_.reset();

        // NOTE: ボスの BT (PlayBGM) が差し替えた BGM も流れているので、シーンの BGM だけでなく全部止める
        GamePlay::Sound::SoundPlayer::StopAllBgm();
    }
    
    void FirstTouchDownMainIsLandScene::OnDrawGui()
    {
        
    }
}
