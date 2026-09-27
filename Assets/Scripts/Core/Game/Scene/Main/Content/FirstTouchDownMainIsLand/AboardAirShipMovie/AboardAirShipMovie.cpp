#include "AboardAirShipMovie.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
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
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"
#include "../../../../../PlayerAvatar/SwordMan/State/ArmStretch/SwordManAvatarArmStretchState.h"
#include "../../../../../PlayerAvatar/SwordMan/State/Walk/SwordManAvatarWalkState.h"
#include "../Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    namespace
    {
        // 甲板のカメラ(1)やプレイヤーの FollowFromBehind(0) より上に出す
        constexpr int OPENING_SHOT_PRIORITY = 100;
        constexpr float DEFAULT_OPENING_SHOT_SECS = 4.0f;
    }

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
        co_await Coroutine::WaitUntil([this] { return isOpeningFinished_ || ShouldStop(); });
        if (ShouldStop())
            co_return;
        
        // 1度目の飛行機の移動
        const auto firstMoveTween = tweeny::from(Context()->AirShip()->Transform().GetWorldPos())
                                    .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
                                    .during(Context()->AirShipFirstMoveDuring_msecs())
                                    .via(Tween::Ease(EaseType::Linear));
        
        const auto airShipBody = Context()->AirShip()->Components().Catch<NanamiEngine::Module::Component::RigidBody>().lock();
        if (airShipBody)
        {
            co_await Coroutine::WaitForTweenBody(*airShipBody, Context()->AirShip()->Transform(), firstMoveTween);
        }
        else
        {
            co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), firstMoveTween);
        }
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
        {
            co_await Coroutine::WaitForTweenBody(*airShipBody, Context()->AirShip()->Transform(), secondMoveTween);
        }
        else
        {
            co_await Coroutine::WaitForTween(Context()->AirShip()->Transform(), secondMoveTween);
        }
        if (ShouldStop())
            co_return;
        
        playerAvatar_.lock()->PlayerTransform().SetParent(std::weak_ptr<GameObject::IGameObject>(), true);
        context_.lock()->BoundryAirShipCollider().OnDestroy();
        LoosenDeckProps();
    }

    void AboardAirShipMovie::LoosenDeckProps() const
    {
        const auto deckProps = Context()->AirShipDeckProps();
        if (!deckProps)
            return;

        const auto loosen = [](const auto& self, GameObject::IGameObject& gameObject) -> void
        {
            for (const auto& weak : gameObject.Components().Catches<NanamiEngine::Module::Component::RigidBody>())
            {
                if (const auto body = weak.lock(); body && body->MotionType() == NanamiEngine::Module::Physics::MotionType::Kinematic)
                {
                    body->SetMotionType(NanamiEngine::Module::Physics::MotionType::Dynamic);
                }
            }
            for (const auto& child : gameObject.Transform().GetChildren())
            {
                self(self, *child);
            }
        };
        loosen(loosen, *deckProps);
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieStagingAsync()
    {
        using namespace PlayerAvatar::SwordMan::State;
        
        playerAvatar_.lock()->GetEventSceneStateMachine().OnDisable();
        
        // 船と島を外から映すカット
        co_await AirShipMovieOpeningShotsAsync();
        if (ShouldStop())
            co_return;

        if (const auto brain = Context()->CameraBrain())
        {
            brain->SnapToVirtualCamera(*Context()->SecondVirtualCamera());
        }
        Context()->SecondVirtualCamera()->OnDisable();

        const auto player = playerAvatar_.lock();
        player->PlayerTransform().LookAtY(Context()->PlayerFirstMoveTarget().GetWorldPos());

        // Playerを操作可能に変更
        player->GetEventSceneStateMachine().OnEnable();
        player->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
        isOpeningFinished_ = true;
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieOpeningShotsAsync()
    {
        // タイトルロゴは最初のカットで出し、次のカットへ移るときに消す
        context_.lock()->TitleLogo().lock()->Entity().lock()->SetEnable(true);

        const auto shotsRoot = Context()->OpeningShots();
        const auto shots = shotsRoot ? shotsRoot->Transform().GetChildren() : std::vector<std::shared_ptr<GameObject::IGameObject>>();
        const std::vector<float> durations_secs = Context()->OpeningShotDurations_secs();
        for (std::size_t i = 0; i < shots.size(); ++i)
        {
            const float duration_secs = i < durations_secs.size() ? durations_secs[i] : DEFAULT_OPENING_SHOT_SECS;
            co_await AirShipMovieOpeningShotAsync(shots[i], duration_secs);
            if (ShouldStop())
                co_return;

            if (i == 0)
            {
                FadeOutTitleLogo();
            }
        }

        if (shots.empty())
        {
            FadeOutTitleLogo();
        }
    }

    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieOpeningShotAsync(
        const std::shared_ptr<GameObject::IGameObject> shot, const float duration_secs)
    {
        const auto camera = shot->Components().Catch<CineMachine::CineMachineVirtualCamera>().lock();
        if (!camera)
            co_return;

        auto& transform = shot->Transform();
        const glm::vec3 fromPos = transform.GetWorldPos();
        const glm::quat fromRot = transform.GetWorldRot();
        // NOTE: 行き先は子なので、カメラを動かす前に読んでおく
        const auto children = transform.GetChildren();
        const glm::vec3 toPos = children.empty() ? fromPos : children.front()->Transform().GetWorldPos();
        const glm::quat toRot = children.empty() ? fromRot : children.front()->Transform().GetWorldRot();

        // カットなので補間せずに切り替え、以降も Brain の追従を挟まずに動かす
        camera->SetImmediateApply(true);
        camera->SetPriority(OPENING_SHOT_PRIORITY);
        if (const auto brain = Context()->CameraBrain())
            brain->SnapToVirtualCamera(*camera);

        // NOTE: シーン切り替え直後は DeltaTime が 0 で凍っているので、動き出してから数える
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });

        float elapsed_secs = 0.0f;
        while (elapsed_secs < duration_secs)
        {
            co_await Coroutine::WaitYield();
            if (ShouldStop())
                co_return;

            elapsed_secs += Time::DeltaTime();
            const float t = std::clamp(elapsed_secs / duration_secs, 0.0f, 1.0f);
            transform.SetWorldPos(glm::mix(fromPos, toPos, t));
            transform.SetWorldRot(glm::slerp(fromRot, toRot, t));
        }
        camera->OnDisable();
    }
    
    void AboardAirShipMovie::FadeOutTitleLogo() const
    {
        const auto titleLogoBlendRenderer = context_.lock()->TitleLogo().lock()->Components().Catch<NanamiUi::BlendAnimationRenderer>();
        titleLogoBlendRenderer.lock()->SetAddBlendRate_secs(-titleLogoBlendRenderer.lock()->GetAddBlendRate_secs());
    }
}
