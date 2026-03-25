#include "VirtualCameraFollowBehaviour.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

void CineMachine::Behaviour::VirtualCameraFollowBehaviour::SetTarget(
    const std::shared_ptr<GameObject::IGameObject>& followTarget)
{
    followTarget_ = followTarget;
}

glm::vec3 CineMachine::Behaviour::VirtualCameraFollowBehaviour::MoveTargetPosition() const noexcept
{
    return followTarget_->TransformRef().GetWorldPos() + followOffset_;
}

void CineMachine::Behaviour::VirtualCameraFollowBehaviour::OnUpdate()
{
    if (!followTarget_)
        return;

    Transform().SetWorldPos(MoveTargetPosition());
}

void CineMachine::Behaviour::VirtualCameraFollowBehaviour::OnDebugRender()
{
    if (!followTarget_)
        return;
    
    if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
    {
        Transform().SetWorldPos(MoveTargetPosition());
    }
}
