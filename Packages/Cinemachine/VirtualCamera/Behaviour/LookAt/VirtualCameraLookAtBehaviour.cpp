#include "VirtualCameraLookAtBehaviour.h"

#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::SetTarget(
    const std::shared_ptr<GameObject::IGameObject>& target)
{
    target_ = target;
}

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::SetOffsetPos(const glm::vec3 lookAtTargetOffset)
{
    lookAtTargetOffset_ = lookAtTargetOffset;
}

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::OnAwake()
{
    
}

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::OnUpdate()
{
    LookAtTarget();
}

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::OnDebugRender()
{
    if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
    {
        LookAtTarget();
    }
}

void CineMachine::Behaviour::VirtualCameraLookAtBehaviour::LookAtTarget() const
{
    if (!target_)
        return;
        
    const glm::vec3 cameraPos = Transform().GetWorldPos();
    const glm::vec3 targetPos = target_->Transform().GetWorldPos() + lookAtTargetOffset_;

    constexpr auto up = glm::vec3(0, 1, 0);

    glm::vec3 toTarget = targetPos - cameraPos;
    // カメラと注視対象の座標が一致する場合、normalize(0,0,0)はNaNになるためフォールバック
    if (glm::dot(toTarget, toTarget) < 0.0001f)
        toTarget = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 forward = glm::normalize(toTarget);

    glm::vec3 rightAxis = glm::cross(up, forward);
    // forwardがワールドupと平行な場合、crossがゼロベクトルになるため別軸でフォールバック
    if (glm::dot(rightAxis, rightAxis) < 0.0001f)
        rightAxis = glm::cross(glm::vec3(0, 0, 1), forward);
    const glm::vec3 right = glm::normalize(rightAxis);
    const glm::vec3 correctedUp = glm::cross(forward, right);

    glm::mat3 lookAtMatrix;
    lookAtMatrix[0] = right;
    lookAtMatrix[1] = correctedUp;
    lookAtMatrix[2] = forward;

    const glm::quat lookAtRotation = glm::normalize(glm::quat_cast(lookAtMatrix));
    Transform().SetWorldRot(lookAtRotation);
}
