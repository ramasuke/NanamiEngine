#include "ILockOnTarget.h"

#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore::PlayerAvatar
{
    glm::vec3 LockOnPositionOf(NanamiEngine::Module::GameObject::IGameObject& target)
    {
        if (const auto lockOnTarget = target.Components().Catch<ILockOnTarget>().lock())
            return lockOnTarget->LockOnPosition();

        return target.Transform().GetWorldPos();
    }
}
