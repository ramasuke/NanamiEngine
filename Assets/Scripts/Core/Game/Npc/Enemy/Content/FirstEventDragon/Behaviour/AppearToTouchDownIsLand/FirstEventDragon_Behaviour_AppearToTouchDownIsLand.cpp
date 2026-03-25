#include "FirstEventDragon_Behaviour_AppearToTouchDownIsLand.h"

#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/WaitForTween.h"
#include "../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../Animation/FirstEventDragon_AnimationType.h"

namespace GameCore::Npc::Enemy::FirstEventDragon
{
    Behaviour::TickStatus AppearToTouchDownIsLand::DoTick(const Behaviour::Action::TickContext& context)
    {
        if (isFinished_)
        {
            isRunning_  = false;
            isFinished_ = false;
            return Behaviour::TickStatus::Success;
        }
        
        if (!isRunning_)
        {
            isRunning_ = true;
            Coroutine::StartCoroutine(AppearToTouchDownIsLandAsync(context));
        }
        return Behaviour::TickStatus::Running;
    }

    Coroutine::Task<void> AppearToTouchDownIsLand::AppearToTouchDownIsLandAsync(const Behaviour::Action::TickContext context)
    {
        co_await ToDestroyAirShipMovieAsync(context);
        co_await ToTouchDownIsLandAsync    (context);
        isFinished_ = true;
    }

    Coroutine::Task<void> AppearToTouchDownIsLand::ToDestroyAirShipMovieAsync(const Behaviour::Action::TickContext context)
    {
        //context.EnemyAnimator().Param<int>(Behaviour::ANIMATOR_PARAM_NAME).Set(static_cast<int>(AnimationType::FlyingIdle));
        for (const auto& throughRoute : toDestroyAirShipRoute_->Get())
        {
            co_await StartTween(throughRoute, context);
        }
    }

    Coroutine::Task<void> AppearToTouchDownIsLand::ToTouchDownIsLandAsync(const Behaviour::Action::TickContext context)
    {
        for (const auto& throughRoute : toTouchDownIsLandRoute_->Get())
        {
            co_await StartTween(throughRoute, context);
        }
    }

    Coroutine::Task<void> AppearToTouchDownIsLand::StartTween(
        const Data::EventNpcWalkingRoute::RoutePoint& throughRoute, const Behaviour::Action::TickContext context)
    {
        const auto tween = tweeny::from(context.EnemyTransform().GetWorldPos())
                           .to(throughRoute.Position())
                           .during(throughRoute.Duration_msecs())
                           .via(Tween::Ease(EaseType::Linear));
        
        co_await Coroutine::WaitForTween(context.EnemyTransform(), tween);
    }

    void AppearToTouchDownIsLand::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("toDestroyAirShipRoute_", toDestroyAirShipRoute_);
        ImGuiHelper::OnDrawInputField("toTouchDownIsLandRoute_", toTouchDownIsLandRoute_);
    }
}
