#include "PlayerAvatarCameraGroupBase.h"

#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"

namespace GameCore::PlayerAvatar
{
    std::weak_ptr<CineMachine::CineMachineVirtualCamera> PlayerAvatarCameraGroupBase::FollowFromBehind() const
    {
        return followFromBehindCamera_.get();
    }
    
    void PlayerAvatarCameraGroupBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera)
    {
        if (currentCamera_.expired())
            return;
        
        currentCamera_.lock()->OnDisable();
        currentCamera_ = camera;
        currentCamera_.lock()->SetPriority(ENABLE_CURRENT_CAMERA_PRIORITY);
    }

    void PlayerAvatarCameraGroupBase::Init(const std::shared_ptr<GameObject::IGameObject>& playerAvatarObject)
    {
        followFromBehindCamera_->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()->SetTarget(playerAvatarObject);
        currentCamera_ = followFromBehindCamera_.get();
    }
}
