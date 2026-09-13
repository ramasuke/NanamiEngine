#include "PlayerAvatarStateCondition.h"

#include "../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Engine/Module/Physics/Component/Listener/Collision/Engine_Physics_CollisionListener.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../../../GamePlay/PlayerAvatar/WakeUpArea/WakeUpArea.h"
#include "../../../../../GamePlay/Prop/Canon/Prop_Canon.h"

namespace GameCore::PlayerAvatar::State
{
    PlayerAvatarStateCondition::PlayerAvatarStateCondition(
        const std::shared_ptr<IPlayerAvatarStateContext>& stateContext)
        : stateContext_(stateContext)
    {
    }

    bool PlayerAvatarStateCondition::IsGround() const
    {
        Physics::LayerMask mask;
        Physics::AddLayer(mask, Physics::Layer::Default);

        //NOTE: Rayだと段差の縁や地形の隙間で抜けて Floating になるため球判定
        const float radius = stateContext_->GroundCheckRadius();
        return Physics::SphereCast(stateContext_->PlayerAvatarFeatStepPos() + glm::vec3(0.0f, stateContext_->GroundCheckUpOffset() + radius, 0.0f),
                                   radius,
                                   glm::vec3(0, -1, 0), stateContext_->GroundCheckDistance(),
                                   mask).Hit();
    }

    bool PlayerAvatarStateCondition::IsChattable() const
    {
        return !stateContext_->ChattableArea().CatchChatTarget().expired();
    }

    bool PlayerAvatarStateCondition::CanWakeUp() const
    {
        return !stateContext_->WakeUpArea().CatchWakeUpTarget().expired();
    }

    bool PlayerAvatarStateCondition::CanUseCannon() const
    {
        const auto collisionListener = stateContext_->PlayerAvatarObject()->Components().Catch<Component::CollisionListener>();
        for (const auto& gameobject : collisionListener.lock()->GetCollisionStayObjects() | std::views::values)
        {
            if (!gameobject.lock()->Components().Catch<GamePlay::Prop::Canon>().expired())
            {
                return true;
            }
        }
        return false;
    }
}
