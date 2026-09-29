#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace NanamiEngine::CineMachine
{
    class NANAMI_API ILockOnCameraTarget
    {
    public:
        virtual ~ILockOnCameraTarget() = default;
        [[nodiscard]] virtual glm::vec3 LockOnPosition() = 0;

        [[nodiscard]] static glm::vec3 PositionOf(Module::GameObject::IGameObject& target)
        {
            if (const auto lockOnTarget = target.Components().Catch<ILockOnCameraTarget>().lock())
                return lockOnTarget->LockOnPosition();
         
            return target.Transform().GetWorldPos();
        }
    };
}
