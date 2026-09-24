#include "IPlayerAvatarStateContext.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"

namespace GameCore::PlayerAvatar
{
    Component::Animator& IPlayerAvatarStateContext::PlayerAvatarAnimator() const
    {
        return *PlayerAvatarObject()->Components().Catch<Component::Animator>().lock();
    }
}
