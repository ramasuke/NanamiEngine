#include "PlayerAvatarCameraGroupBase.h"

#include "../../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
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
        followFromBehindCamera_ = GameObject::CatchChild<CineMachine::CineMachineVirtualCamera>(Entity(), followFromBehindCameraName_);
        
        auto& followCamera = followFromBehindCamera_->Components();
        auto weakCamera =  followCamera.Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>();
        auto camera = weakCamera.lock();
        camera->SetTarget(playerAvatarObject);
        currentCamera_ = followFromBehindCamera_.get();
        
        auto shakeCamera = followCamera.Catch<CineMachine::Behaviour::ShakeCameraBehaviour>().lock();
        shakeCamera;
    }

    void PlayerAvatarCameraGroupBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("followFromBehindCameraName_", followFromBehindCameraName_);
        ImGuiHelper::OnDrawInputField("followFromBehindCamera_", followFromBehindCamera_);
    }
}
