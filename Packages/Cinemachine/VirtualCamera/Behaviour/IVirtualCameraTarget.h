#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace NanamiEngine::CineMachine
{
    class NANAMI_API IVirtualCameraTarget
    {
    public:
        virtual ~IVirtualCameraTarget() = default;
        [[nodiscard]] virtual glm::vec3 CameraTargetPosition() const = 0;

        [[nodiscard]] static glm::vec3 PositionOf(Module::GameObject::IGameObject& target)
        {
            if (const auto cameraTarget = target.Components().Catch<IVirtualCameraTarget>().lock())
                return cameraTarget->CameraTargetPosition();
            
            return target.Transform().GetWorldPos();
        }
    };
}
