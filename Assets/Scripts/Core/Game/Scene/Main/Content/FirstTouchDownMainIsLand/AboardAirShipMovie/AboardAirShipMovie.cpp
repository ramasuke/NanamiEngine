#include "AboardAirShipMovie.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSubscription/Coroutine_WaitForSubscription.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTweenBody/Coroutine_WaitForTweenBody.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/NanamiUI/BlendAnimationRenderer/BlendAnmiationRenderer.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTweenV/Coroutine_WaitForTweenV.h"
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"
#include "../../../../../PlayerAvatar/SwordMan/State/ArmStretch/SwordManAvatarArmStretchState.h"
#include "../../../../../PlayerAvatar/SwordMan/State/Walk/SwordManAvatarWalkState.h"
#include "../Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    AboardAirShipMovie::AboardAirShipMovie(
          const std::weak_ptr<IPlayerAvatar>& playerAvatar
        , const std::shared_ptr<FirstTouchDownMainIsLandSceneContext>& context)
        : playerAvatar_(playerAvatar)
        , context_     (context     )
    {
        
    }

    Coroutine::Task<void> AboardAirShipMovie::PlayAsync(const std::shared_ptr<AboardAirShipMovie> movie)
    {
        co_await movie->Invoke();
    }

    Coroutine::Task<void> AboardAirShipMovie::StagingAsync(const std::shared_ptr<AboardAirShipMovie> movie)
    {
        co_await movie->AirShipMovieStagingAsync();
    }

    bool AboardAirShipMovie::ShouldStop() const
    {
        return isCancelled_ || playerAvatar_.expired() || context_.expired();
    }

    Coroutine::Task<void> AboardAirShipMovie::Invoke()
    {
        Coroutine::StartCoroutine(StagingAsync(shared_from_this()));
        co_await AboardAirShipMovieMoveAirShipAsync();
    }

    Coroutine::Task<void> AboardAirShipMovie::AboardAirShipMovieMoveAirShipAsync()
    {
        co_await Coroutine::WaitForSeconds(20.0f);
        if (ShouldStop())
            co_return;
        
        // 1度目の飛行機の移動
        const auto firstMoveTween = tweeny::from(Context()->AirShip()->Transform().GetWorldPos())
                                    .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
                                    .during(Context()->AirShipFirstMoveDuring_msecs())
                                    .via(Tween::Ease(EaseType::Linear));
        
        // NOTE: 物理と同じ固定ステップで動かす。毎フレーム動かすと、固定ステップで補間されるプレイヤー(カメラ)とずれてカクつく
        const auto airShipBody = Context()->AirShip()->Components().Catch<NanamiEngine::Module::Component::RigidBody>().lock();
        if (airShipBody)
            co_await Coroutine::WaitForTweenBody(*airShipBody, Context()->AirShip()->Transform(), firstMoveTween);
        else
            co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), firstMoveTween);
        if (ShouldStop())
            co_return;
    
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
        
        if (airShipBody)
            co_await Coroutine::WaitForTweenBody(*airShipBody, Context()->AirShip()->Transform(), secondMoveTween);
        else
            co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), secondMoveTween);
        if (ShouldStop())
            co_return;
        
        playerAvatar_.lock()->PlayerTransform().SetParent(std::weak_ptr<GameObject::IGameObject>(), true);
        context_.lock()->BoundryAirShipCollider().OnDestroy();
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieStagingAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;

        
        // Playerの操作不可能に変更
        playerAvatar_.lock()->GetEventSceneStateMachine().OnDisable();
        
        // 一度目のカメラ移動
        co_await AirShipMovieFirstCameraMoveAsync();
        if (ShouldStop())
            co_return;
        
        // カメラの切り替え
        Context()->FirstVirtualCamera()->OnDisable();
        Context()->CameraBrain()->ApplyVirtualCameraMatrix();
        
        // Playerの歩き
        context_.lock()->TitleLogo().lock()->Entity().lock()->SetEnable(true);
        co_await AirShipMovieWalkPlayerAsync      ();
        if (ShouldStop())
            co_return;
        // PlayerのArmStretch
        co_await AirShipMovieArmStretchPlayerAsync();
        if (ShouldStop())
            co_return;

        StartFadeInUi();
        
        // Playerを操作可能に変更
        playerAvatar_.lock()->GetEventSceneStateMachine().OnEnable();
        playerAvatar_.lock()->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
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
        const auto player = playerAvatar_.lock();
        
        player->PlayerTransform().LookAtY(Context()->PlayerFirstMoveTarget().GetWorldPos());
        player->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Walk);
        player->GetEventSceneStateMachine().OnDisable();

        const auto tween = tweeny::from(player->PlayerTransform().GetWorldPos())
            .to(Context()->PlayerFirstMoveTarget().GetWorldPos())
            .during(1500.0f)
            .via(Tween::Ease(EaseType::Linear));
        co_await Coroutine::WaitForTween(player->PlayerTransform(), tween);
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieArmStretchPlayerAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;
        playerAvatar_.lock()->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::ArmStretch);
        co_await Coroutine::WaitForSeconds(static_cast<float>(Context()->PlayerArmStretchDuring_msecs()) / 1000);
        if (ShouldStop())
            co_return;
        
        Context()->SecondVirtualCamera()->OnDisable();
    }

    void AboardAirShipMovie::StartFadeInUi() const
    {
        const auto titleLogoBlendRenderer = context_.lock()->TitleLogo().lock()->Components().Catch<NanamiUi::BlendAnimationRenderer>();
        titleLogoBlendRenderer.lock()->SetAddBlendRate_secs(-titleLogoBlendRenderer.lock()->GetAddBlendRate_secs());
    }
}
