#include "PlayerAvatarStateCondition.h"

#include "DxLib.h"
#include "../../../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Engine/Module/Physics/Component/Listener/Collision/Engine_Physics_CollisionListener.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../../../GamePlay/Prop/AirShip/Prop_AirShip.h"
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
       
        return Physics::Raycast(stateContext_->PlayerAvatarFeatStepPos(),
                                glm::vec3(0, -1, 0), 5.3f,
                                mask).Hit();
    }

    bool PlayerAvatarStateCondition::IsChattable() const
    {
        return !stateContext_->ChattableArea().CatchChatTarget().expired();
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
