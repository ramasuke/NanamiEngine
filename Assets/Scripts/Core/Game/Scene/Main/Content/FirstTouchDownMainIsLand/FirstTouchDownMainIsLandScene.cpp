#include "FirstTouchDownMainIsLandScene.h"

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/WaitForTween.h"
#include "../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../PlayerAvatar/Factory/PlayerAvatarFactory.h"
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
        
        // PlayerAvatarの生成
        playerAvatar_ = PlayerAvatar::Factory::SummonSwordManAvatar(
              Context()->SummonPlayerAvatarPrefab()
            , Context()->PlayerSpawnPoint()
            , Context()->AirShip());
    }
    
    void FirstTouchDownMainIsLandScene::OnEnter()
    {
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        playerAvatar_.lock()->Transform().SetLocalRot({glm::vec3{0.0f, 90.0f, 0.0f}});
        
        //PlayerAvatarの初期化
        const auto inputAction = std::make_shared<PlayerAvatar::RequireType::InputAction<PlayerAvatar::SwordMan::SwordManAvatarTraits>>();
        const auto summonAvatarStatus = std::make_shared<ContextT::SummonAvatarStatus>(Context()->PlayerAvatarInitStatus());
        auto stateMachine = PlayerAvatar::SwordMan::CreateStateMachine(summonAvatarStatus, inputAction, playerAvatar_.lock(), Context()->CameraGroup());
        playerAvatar_.lock()->Init(summonAvatarStatus, std::move(stateMachine), inputAction, Context()->CameraGroup());
        playerStatusPresenter_ = std::make_unique<PlayerAvatar::SwordMan::StatusPresenter>(
            *Context()->PlayerStatusUI().lock(),
            *summonAvatarStatus);

        // 船を降りるまでのMovieを開始
        aboardAirShipMovie_ = std::make_unique<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        Coroutine::StartCoroutine(aboardAirShipMovie_->ToTask());
    }
    
    void FirstTouchDownMainIsLandScene::OnExit()
    {
        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
    }
    
    void FirstTouchDownMainIsLandScene::Dispose()
    {
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
    }
    
    void FirstTouchDownMainIsLandScene::OnDrawGui()
    {
        
    }
}
