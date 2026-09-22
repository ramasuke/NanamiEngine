#include "ILockOnTarget.h"

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LockOn/LockOnPositionResolver.h"

namespace GameCore::PlayerAvatar
{
    glm::vec3 LockOnPositionOf(NanamiEngine::Module::GameObject::IGameObject& target)
    {
        if (const auto lockOnTarget = target.Components().Catch<ILockOnTarget>().lock())
            return lockOnTarget->LockOnPosition();
        if (const auto lockOnPart = target.Components().Catch<ILockOnPart>().lock())
            return lockOnPart->LockOnPosition();

        return target.Transform().GetWorldPos();
    }
}

namespace
{
    const bool lockOnPositionResolverRegistered = []
    {
        NanamiEngine::CineMachine::Behaviour::SetLockOnPositionResolver(&GameCore::PlayerAvatar::LockOnPositionOf);
        return true;
    }();
}
