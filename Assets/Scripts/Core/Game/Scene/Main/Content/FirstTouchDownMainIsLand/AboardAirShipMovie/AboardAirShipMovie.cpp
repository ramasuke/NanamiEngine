#include "AboardAirShipMovie.h"

#include "../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/WaitForTween.h"
#include "../../../../../../../../../Engine/Module/NanamiUI/BlendAnimationRenderer/BlendAnmiationRenderer.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/WaitForSeconds.h"
#include "../../../Engine/Core/Coroutine/Awaitable/WaitForTweenV/WaitForTweenV.h"
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"
#include "../../../../../PlayerAvatar/SwordMan/State/ArmStretch/SwordManAvatarArmStretchState.h"
#include "../../../../../PlayerAvatar/SwordMan/State/Walk/SwordManAvatarWalkState.h"
#include "../Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    AboardAirShipMovie::AboardAirShipMovie(
          std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::shared_ptr<FirstTouchDownMainIsLandSceneContext>& context)
        : playerAvatar_(playerAvatar)
        , context_     (context     )
    {
        
    }

    Coroutine::Task<void> AboardAirShipMovie::Invoke()
    {
        Coroutine::StartCoroutine(AirShipMovieStagingAsync());
        co_await AboardAirShipMovieMoveAirShipAsync();
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AboardAirShipMovieMoveAirShipAsync()
    {
        // 1度目の飛行機の移動
        const auto firstMoveTween = tweeny::from(Context()->AirShip()->TransformRef().GetWorldPos())
                                    .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
                                    .during(Context()->AirShipFirstMoveDuring_msecs())
                                    .via(Tween::Ease(EaseType::Linear));
        
        co_await Coroutine::WaitForTween(Context()->AirShip()->TransformRef(), firstMoveTween);
    
        // 2度目の飛行機の移動と回転
        const auto secondMoveTween = tweeny::from(
                Context()->AirShip()->TransformRef().GetWorldPos(),
                Context()->AirShip()->TransformRef().GetWorldRot())
             .to(
                 Context()->AirShipSecondMoveFromTarget().GetWorldPos(),
                 Context()->AirShipSecondMoveFromTarget().GetWorldRot()
             )
             .during(Context()->AirShipSecondMoveDuring_msecs())
             .via(Tween::Ease(EaseType::OutQuad), Tween::Ease(EaseType::OutQuad));
        
        co_await Coroutine::WaitForTween(Context()->AirShip()->TransformRef(), secondMoveTween);
        
        playerAvatar_.lock()->Transform().SetParent(std::weak_ptr<GameObject::IGameObject>(), true);
        context_.lock()->BoundryAirShipCollider().OnDestroy();
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieStagingAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;
    
        // Playerの操作不可能に変更
        playerAvatar_.lock()->GetEventSceneStateMachine().OnDisable();
        
        // 一度目のカメラ移動
        co_await AirShipMovieFirstCameraMoveAsync();
        
        // カメラの切り替え
        Context()->FirstVirtualCamera()->OnDisable();
        Context()->CameraBrain()->ApplyVirtualCameraMatrix();
        
        // Playerの歩き
        context_.lock()->TitleLogo().lock()->Entity().lock()->SetEnable(true);
        co_await AirShipMovieWalkPlayerAsync      ();
        // PlayerのArmStretch
        co_await AirShipMovieArmStretchPlayerAsync();

        StartFadeInUi();
        
        // Playerを操作可能に変更
        playerAvatar_.lock()->GetEventSceneStateMachine().OnEnable();
        playerAvatar_.lock()->GetEventSceneStateMachine().OnChangeState(typeid(SwordManAvatarIdleState));
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieFirstCameraMoveAsync()
    {
        const auto firstVirtualCameraFollowBehaviour = Context()->FirstVirtualCamera()->Components().Catch<CineMachine::Behaviour::VirtualCameraFollowBehaviour>();
        const auto targetDirection = Context()->VirtualCameraFirstMoveTarget()->TransformRef().GetWorldPos() - Context()->FirstVirtualCamera()->Transform().GetWorldPos() + firstVirtualCameraFollowBehaviour.lock()->followOffset_;
        const auto firstMoveTween = tweeny::from(firstVirtualCameraFollowBehaviour.lock()->followOffset_)
                                    .to(targetDirection)
                                    .during(Context()->VirtualCameraFirstMoveTargetDuring_msecs())
                                    .via(Tween::Ease(EaseType::Linear));
        co_await Coroutine::WaitForTweenV(firstVirtualCameraFollowBehaviour.lock()->followOffset_, firstMoveTween);    
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieWalkPlayerAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;
        playerAvatar_.lock()->Transform().LookAtY(Context()->PlayerFirstMoveTarget().GetWorldPos());
        constexpr auto playerFirstMoveFrameResolution = 180;
        playerAvatar_.lock()->GetEventSceneStateMachine().OnChangeState(typeid(SwordManAvatarWalkState));
        const auto moveDirection = Context()->PlayerFirstMoveTarget().GetWorldPos() - playerAvatar_.lock()->Transform().GetWorldPos();
        for (int i = 0; i < playerFirstMoveFrameResolution; i++)
        {
            const auto playerAvatar = playerAvatar_.lock(); 
            playerAvatar->Transform().SetWorldPos(playerAvatar->Transform().GetWorldPos() + moveDirection / static_cast<float>(playerFirstMoveFrameResolution));
            co_await Coroutine::WaitForSeconds(static_cast<float>(Context()->PlayerFirstMoveDuring_msecs()) / 1000 / playerFirstMoveFrameResolution);
        }
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieArmStretchPlayerAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;
        playerAvatar_.lock()->GetEventSceneStateMachine().OnChangeState(typeid(SwordManAvatarArmStretchState));
        co_await Coroutine::WaitForSeconds(static_cast<float>(Context()->PlayerArmStretchDuring_msecs()) / 1000);
        Context()->SecondVirtualCamera()->OnDisable();
    }

    void AboardAirShipMovie::StartFadeInUi()
    {
        context_.lock()->ActionControlWayUI().lock()->SetEnable(true);
        
        const auto titleLogoBlendRenderer_ = context_.lock()->TitleLogo().lock()->Components().Catch<NanamiUi::BlendAnimationRenderer>();
        titleLogoBlendRenderer_.lock()->SetAddBlendRate_secs(-titleLogoBlendRenderer_.lock()->GetAddBlendRate_secs());
    }
}
