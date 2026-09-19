#include "PlayerAvatarStateBase.h"

#include "../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"

namespace GameCore::PlayerAvatar
{
    Component::Animator& CatchPlayerAvatarAnimator(GameObject::IGameObject& playerAvatar)
    {
        return *playerAvatar.Components().Catch<Component::Animator>().lock();
    }
}
