#include "SwordManAvatarStateClimbToTop.h"

#include "../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/WaitForTween.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarStateClimbToTop::DoEnter()
    {
        // ClimbingAsync();
    }

    void SwordManAvatarStateClimbToTop::DoUpdate()
    {
        
    }

    void SwordManAvatarStateClimbToTop::DoExit()
    {
        
    }

    // Coroutine::Task<void> SwordManAvatarStateClimbToTop::ClimbingAsync()
    // {
    //     const auto featToPlayerDirection =  FeatStepPos() - Transform().GetWorldPos();
    //     const auto featToPlayerTopDirection = glm::vec3(featToPlayerDirection.x, 0.0f, featToPlayerDirection.z); 
    //     
    //     const auto firstMoveTween = tweeny::from(Context()->AirShip()->TransformRef().GetWorldPos())
    //                                 .to(Context()->AirShipFirstMoveFromTarget().GetWorldPos())
    //                                 .during(Context()->AirShipFirstMoveDuring_msecs())
    //                                 .via(Tween::Ease(EaseType::Linear));
    //     
    //     co_await Coroutine::WaitForTween(Context()->AirShip()->TransformRef(), firstMoveTween);
    // }
}
