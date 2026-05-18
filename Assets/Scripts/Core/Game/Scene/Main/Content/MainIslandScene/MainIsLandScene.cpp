#include "MainIsLandScene.h"

#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"

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
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        scene_ = LoadMainScene();
        Context()->Init();
        
         playerAvatar_ = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatar(
            PlayerAvatar::LoadType(),
            glm::vec3{0.0f, 0.0f, 0.0f},
            nullptr,
            Context()->CameraGroup());
    }

    void MainIslandScene::Enter()
    {
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        
        // //PlayerAvatarの初期化
        // const auto inputAction = std::make_shared<PlayerAvatar::RequireType::InputAction<PlayerAvatar::SwordMan::SwordManAvatarTraits>>();
        // const auto summonAvatarStatus = std::make_shared<ContextT::SummonAvatarStatus>(Context()->PlayerAvatarInitStatus());
        // auto stateMachine = PlayerAvatar::SwordMan::CreateStateMachine(summonAvatarStatus, inputAction, playerAvatar_.lock(), Context()->CameraGroup());
        // playerAvatar_.lock()->Init(summonAvatarStatus, std::move(stateMachine), inputAction, Context()->CameraGroup());
        // playerStatusPresenter_ = std::make_unique<PlayerAvatar::SwordMan::StatusPresenter>(
        //     *Context()->PlayerStatusUI().lock(),
        //     *summonAvatarStatus);
        //
        // // 船を降りるまでのMovieを開始
        // aboardAirShipMovie_ = std::make_unique<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        // Coroutine::StartCoroutine(aboardAirShipMovie_->ToTask());
    }

    void MainIslandScene::Dispose()
    {
        
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
