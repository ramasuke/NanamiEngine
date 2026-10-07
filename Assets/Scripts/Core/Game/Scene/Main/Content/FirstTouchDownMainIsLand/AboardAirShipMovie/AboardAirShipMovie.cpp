#include "AboardAirShipMovie.h"

#include <algorithm>

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSubscription/Coroutine_WaitForSubscription.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTweenBody/Coroutine_WaitForTweenBody.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Core/Physics/Physics.h"
#include "Engine/Module/NanamiUI/BlendAnimationRenderer/BlendAnmiationRenderer.h"
#include "Engine/Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Libs/glm/gtc/quaternion.hpp"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../../PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"
#include "../../../../../PlayerAvatar/SwordMan/State/ArmStretch/SwordManAvatarArmStretchState.h"
#include "../../../../../PlayerAvatar/SwordMan/State/Walk/SwordManAvatarWalkState.h"
#include "../Context/FirstTouchDownMainIsLandSceneContext.h"

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    namespace
    {
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

    void AboardAirShipMovie::DockImmediately(FirstTouchDownMainIsLandSceneContext& context)
    {
        const auto airShip = context.AirShip()->Entity().lock();
        if (!airShip)
            return;

        const auto& docked = context.AirShipSecondMoveFromTarget();
        airShip->Transform().SetWorldPos(docked.GetWorldPos());
        airShip->Transform().SetWorldRot(docked.GetWorldRot());
        // NOTE: Kinematic は Transform の差分を速度にして運ばれるので、物理側も瞬間移動させておく
        NanamiEngine::Core::Application::ApplicationBase::Physics().Bodies().SyncTransforms(*airShip);

        context.SecondVirtualCamera()->OnDisable();
        context.BoundryAirShipCollider().OnDestroy();
        LoosenDeckProps(context);
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
        LoosenDeckProps(*Context());
    }

    void AboardAirShipMovie::LoosenDeckProps(FirstTouchDownMainIsLandSceneContext& context)
    {
        const auto deckProps = context.AirShipDeckProps();
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
        
        const NanamiEngine::ControlLock::ScopedLock controlLock(NanamiEngine::ControlLock::Service::Instance().Acquire());

        playerAvatar_.lock()->PlayerTransform().LookAtY(Context()->PlayerFirstMoveTarget().GetWorldPos());
        Context()->SecondVirtualCamera()->OnDisable();

        // 船と島を外から映し、主人公から追従カメラへつなぐカット
        co_await AirShipMovieOpeningShotsAsync();
        if (ShouldStop())
            co_return;

        isOpeningFinished_ = true;
    }
    
    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieOpeningShotsAsync()
    {
        context_.lock()->TitleLogo().lock()->Entity().lock()->SetEnable(true);

        const auto shotsRoot = Context()->OpeningShots();
        const auto shots = shotsRoot ? shotsRoot->Transform().GetChildren() : std::vector<std::shared_ptr<GameObject::IGameObject>>();
        const std::vector<float> durations_secs = Context()->OpeningShotDurations_secs();
        for (std::size_t i = 0; i < shots.size(); ++i)
        {
            const float duration_secs = i < durations_secs.size() ? durations_secs[i] : DEFAULT_OPENING_SHOT_SECS;
            if (i + 1 == shots.size())
                co_await AirShipMovieJoinPlayerCameraShotAsync(shots[i], duration_secs);
            else
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

        // 子の End のカメラへ、カットの長さをかけて Brain の補間で動かす
        const auto children = shot->Transform().GetChildren();
        auto endCamera = children.empty() ? nullptr : children.front()->Components().Catch<CineMachine::CineMachineVirtualCamera>().lock();
        if (!endCamera)
            endCamera = camera;

        camera->SetImmediateApply(true);
        endCamera->SetImmediateApply(true);
        camera->SetPriority(OPENING_SHOT_PRIORITY);
        if (const auto brain = Context()->CameraBrain())
            brain->SnapToVirtualCamera(*camera);

        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (ShouldStop())
            co_return;

        if (endCamera != camera)
        {
            endCamera->SetBlendIn(duration_secs, LibCore::EaseType::Linear);
            endCamera->SetPriority(OPENING_SHOT_PRIORITY + 1);
        }

        float elapsed_secs = 0.0f;
        while (elapsed_secs < duration_secs)
        {
            co_await Coroutine::WaitYield();
            if (ShouldStop())
                co_return;

            elapsed_secs += Time::DeltaTime();
        }
        endCamera->OnDisable();
        camera->OnDisable();
    }

    Coroutine::Task<void> AboardAirShipMovie::AirShipMovieJoinPlayerCameraShotAsync(
        const std::shared_ptr<GameObject::IGameObject> shot, const float duration_secs)
    {
        const auto swordMan = PlayerAvatar::TryWhetherPlayerT<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>(playerAvatar_.lock());
        const auto cameraGroup = swordMan ? swordMan->AvatarCameraGroup().lock() : nullptr;
        const auto followCamera = cameraGroup ? cameraGroup->FollowFromBehind().lock() : nullptr;
        const auto camera = shot->Components().Catch<CineMachine::CineMachineVirtualCamera>().lock();
        if (!followCamera || !camera)
        {
            co_await AirShipMovieOpeningShotAsync(shot, duration_secs);
            co_return;
        }

        auto& transform = shot->Transform();
        const glm::vec3 fromPos = transform.GetWorldPos();
        const auto children = transform.GetChildren();
        const glm::vec3 viaPos = children.empty() ? fromPos : children.front()->Transform().GetWorldPos();
        const float holdRate = Context()->HeroHoldRate();
        const float turnStartRate = Context()->HeroTurnStartRate();
        const float lookHeight = Context()->HeroLookHeight();

        camera->SetImmediateApply(true);
        camera->SetPriority(OPENING_SHOT_PRIORITY);
        if (const auto brain = Context()->CameraBrain())
            brain->SnapToVirtualCamera(*camera);

        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });

        float elapsed_secs = 0.0f;
        while (elapsed_secs < duration_secs)
        {
            co_await Coroutine::WaitYield();
            if (ShouldStop())
                co_return;

            elapsed_secs += Time::DeltaTime();
            const float t = std::clamp(elapsed_secs / duration_secs, 0.0f, 1.0f);
            
            const float move = glm::smoothstep(holdRate, 1.0f, t);
            const glm::vec3 toPos = followCamera->Transform().GetWorldPos();
            const glm::vec3 pos = glm::mix(glm::mix(fromPos, viaPos, move), glm::mix(viaPos, toPos, move), move);

            const glm::vec3 heroPos = playerAvatar_.lock()->PlayerTransform().GetWorldPos() + glm::vec3(0.0f, lookHeight, 0.0f);
            const glm::vec3 toHero = heroPos - pos;
            const glm::quat lookHeroRot = glm::dot(toHero, toHero) > 1e-6f
                ? glm::quatLookAtLH(glm::normalize(toHero), glm::vec3(0.0f, 1.0f, 0.0f))
                : followCamera->Transform().GetWorldRot();
            const float turn = glm::smoothstep(turnStartRate, 1.0f, t);

            transform.SetWorldPos(pos);
            transform.SetWorldRot(glm::slerp(lookHeroRot, followCamera->Transform().GetWorldRot(), turn));
        }
        camera->OnDisable();
    }

    void AboardAirShipMovie::FadeOutTitleLogo() const
    {
        const auto titleLogoBlendRenderer = context_.lock()->TitleLogo().lock()->Components().Catch<NanamiUi::BlendAnimationRenderer>();
        titleLogoBlendRenderer.lock()->SetAddBlendRate_secs(-titleLogoBlendRenderer.lock()->GetAddBlendRate_secs());
    }
}
