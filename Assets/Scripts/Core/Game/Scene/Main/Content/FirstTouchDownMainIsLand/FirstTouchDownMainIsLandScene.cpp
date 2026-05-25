#include "FirstTouchDownMainIsLandScene.h"

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../PlayerAvatar/Presenter/PlayerAvatar_StatusPresenter.h"
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
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        scene_ = LoadMainScene();
        Context()->Init();
        
        //PlayerAvatarの生成
        playerAvatar_ = Context()->PlayerAvatarFactory().SummonSwordManAvatar(
              Context()->PlayerSpawnPoint()
            , Context()->AirShip()->Entity().lock());
    }
    
    void FirstTouchDownMainIsLandScene::Enter()
    {
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        playerAvatar_.lock()->Transform().SetLocalRot({glm::vec3{0.0f, 90.0f, 0.0f}});
        
        //PlayerAvatarの初期化
        using namespace GameCore::PlayerAvatar;
        auto inputAction = std::make_shared<RequireType::InputAction<SwordMan::SwordManAvatarTraits>>();
        auto summonAvatarStatus = std::make_shared<ContextT::SummonAvatarStatus>(Context()->PlayerAvatarInitStatus());
        auto stateMachine = PlayerAvatar::SwordMan::CreateStateMachine(summonAvatarStatus, inputAction, playerAvatar_.lock(), Context()->CameraGroup().Swordman());
        playerAvatar_.lock()->Init(summonAvatarStatus, std::move(stateMachine), inputAction, Context()->CameraGroup().Swordman());

        auto& context = *Context();
        auto playerStatusUi = context.PlayerStatusUI();
        //StatusPresenter
        playerStatusPresenter_ = std::make_unique<SwordMan::StatusPresenter>(
            *playerStatusUi.lock(),
            *summonAvatarStatus);
        
        // 船を降りるまでのMovieを開始
        aboardAirShipMovie_ = std::make_unique<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        Coroutine::StartCoroutine(aboardAirShipMovie_->ToTask());
    }

    void FirstTouchDownMainIsLandScene::Dispose()
    {
        PlayerAvatar::SaveType(*playerAvatar_.lock());
        playerAvatar_.lock()->SaveStatus();
        SaveGameProgression(GameProgresion::MainIsland);
         
        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
    }
    
    void FirstTouchDownMainIsLandScene::OnDrawGui()
    {
        
    }
}
