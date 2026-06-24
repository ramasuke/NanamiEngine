#include "FirstTouchDownMainIsLandScene.h"

#include "../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
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
    }
    
    void FirstTouchDownMainIsLandScene::Enter()
    {
        auto& context = *Context();
        
        GamePlay::Sound::SoundPlayer::PlayBgm(context.BGM());
        //PlayerAvatarの初期化
        using namespace GameCore::PlayerAvatar;
        
        auto inputAction        = std::make_shared<RequireType::InputAction<SwordMan::SwordManAvatarTraits>>();
        auto summonAvatarStatus = std::make_shared<ContextT::SummonAvatarStatus>(Context()->PlayerAvatarInitStatus());

        /** Player生成処理 */
        playerAvatar_ = context.PlayerAvatarFactory().LoadInitedPlayerAvatar(
            PlayerAvatarType::SwordMan,
            context.PlayerSpawnPoint(),
            context.AirShip()->Entity().lock(),
            true);
        playerAvatar_.lock()->PlayerTransform().SetLocalRot({glm::vec3{0.0f, 90.0f, 0.0f}});
        
        // 船を降りるまでのMovieを開始
        aboardAirShipMovie_ = std::make_unique<FirstTouchDownMainIsLand::AboardAirShipMovie>(playerAvatar_, Context());
        Coroutine::StartCoroutine(aboardAirShipMovie_->ToTask());
    }

    void FirstTouchDownMainIsLandScene::DoDispose()
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
