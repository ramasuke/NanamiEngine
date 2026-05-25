#include "AboardAirShipMovie.h"

#include "../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForSubscription/Coroutine_WaitForSubscription.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../../../../../Engine/Module/NanamiUI/BlendAnimationRenderer/BlendAnmiationRenderer.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../Engine/Core/Coroutine/Awaitable/WaitForTweenV/Coroutine_WaitForTweenV.h"
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"
#include "../../../../../PlayerAvatar/SwordMan/State/ArmStretch/SwordManAvatarArmStretchState.h"
#include "../../../../../PlayerAvatar/SwordMan/State/Walk/SwordManAvatarWalkState.h"
#include "../Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    AboardAirShipMovie::AboardAirShipMovie(
          const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
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
        co_await Coroutine::WaitForSeconds(20.0f);
        
        // 1度目の飛行機の移動
        const auto firstMoveTween = tweeny::from(Context()->AirShip()->Transform().GetWorldPos())
                                    .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
                                    .during(Context()->AirShipFirstMoveDuring_msecs())
                                    .via(Tween::Ease(EaseType::Linear));
        
        co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), firstMoveTween);
    
        // 2度目の飛行機の移動と回転
        const auto secondMoveTween = tweeny::from(
                Context()->AirShip()->Transform().GetWorldPos(),
                Context()->AirShip()->Transform().GetWorldRot())
             .to(
                 Context()->AirShipSecondMoveFromTarget().GetWorldPos(),
                 Context()->AirShipSecondMoveFromTarget().GetWorldRot()
             )
             .during(Context()->AirShipSecondMoveDuring_msecs())
             .via(Tween::Ease(EaseType::OutQuad), Tween::Ease(EaseType::OutQuad));
        
        co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), secondMoveTween);
        
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
        const auto targetDirection = Context()->VirtualCameraFirstMoveTarget()->Transform().GetWorldPos() - Context()->FirstVirtualCamera()->Transform().GetWorldPos() + firstVirtualCameraFollowBehaviour.lock()->followOffset_;
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

    // Coroutine::Task<void> AboardAirShipMovie::ArmStretchAsync() const
    // {
    //     using namespace Coroutine;
    //     co_await WaitForObservable(rxcpp::observable());
    //     co_await WaitForSeconds(1.0f);
    //     co_await WaitForSubscription(rxcpp::composite_subscription());
    //     co_await WaitUntil([] { return true; });
    //     co_await WaitYield();
    //     
    //     const auto tween = tweeny::from(Context()->AirShip()->Transform().GetWorldPos())
    //                                 .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
    //                                 .during(Context()->AirShipFirstMoveDuring_msecs())
    //                                 .via(Tween::Ease(EaseType::Linear));
    //     co_await WaitForTween(Context()->AirShip()->Transform(), tween);
    // }

    void AboardAirShipMovie::StartFadeInUi() const
    {
        const auto context = Context();
        const auto wekUi = context->ActionControlWayUI();
        const auto ui = wekUi.lock();
        ui->SetEnable(true);
        
        const auto titleLogoBlendRenderer = context_.lock()->TitleLogo().lock()->Components().Catch<NanamiUi::BlendAnimationRenderer>();
        titleLogoBlendRenderer.lock()->SetAddBlendRate_secs(-titleLogoBlendRenderer.lock()->GetAddBlendRate_secs());
    }
}
