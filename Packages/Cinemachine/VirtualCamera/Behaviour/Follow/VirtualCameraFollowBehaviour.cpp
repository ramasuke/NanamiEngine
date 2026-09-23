#include "VirtualCameraFollowBehaviour.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../IVirtualCameraTarget.h"
#include "../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

void CineMachine::Behaviour::VirtualCameraFollowBehaviour::SetTarget(
    const std::shared_ptr<GameObject::IGameObject>& followTarget)
{
    followTarget_ = followTarget;
}

glm::vec3 CineMachine::Behaviour::VirtualCameraFollowBehaviour::MoveTargetPosition() const noexcept
{
    return CameraTargetPositionOf(*followTarget_.get()) + followOffset_;
}

void CineMachine::Behaviour::VirtualCameraFollowBehaviour::OnCameraUpdate()
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
#pragma endregion
