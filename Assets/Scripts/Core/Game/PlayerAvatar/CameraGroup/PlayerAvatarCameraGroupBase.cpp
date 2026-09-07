#include "PlayerAvatarCameraGroupBase.h"

#include "../../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/LockOn/LockOnCameraBehaviour.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"

namespace GameCore::PlayerAvatar
{
    std::weak_ptr<CineMachine::CineMachineVirtualCamera> PlayerAvatarCameraGroupBase::FollowFromBehind() const
    {
        return followFromBehindCamera_.get();
    }

    std::weak_ptr<CineMachine::CineMachineVirtualCamera> PlayerAvatarCameraGroupBase::LockOnCamera() const
    {
        return lockOnCamera_;
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
        const auto lockOnCamera = GameObject::CatchChild<CineMachine::CineMachineVirtualCamera>(Entity(), lockOnCameraName_);
        lockOnCamera->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->SetFollowTarget(playerAvatarObject);
        // ロックオン開始まではカメラ優先度を最低にしておき、FollowFromBehind の妨げにならないようにする
        lockOnCamera->OnDisable();
        lockOnCamera_ = lockOnCamera;
    }

    void PlayerAvatarCameraGroupBase::EngageLockOn(const std::shared_ptr<GameObject::IGameObject>& target)
    {
        if (!target || lockOnCamera_.expired())
            return;

        lockOnCamera_.lock()->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->SetLockOnTarget(target);
        ChangeCamera(LockOnCamera());
        lockOnTarget_ = target;
        isLockedOn_ = true;
    }

    void PlayerAvatarCameraGroupBase::ReleaseLockOn()
    {
        if (!isLockedOn_)
            return;

        if (!lockOnCamera_.expired())
            lockOnCamera_.lock()->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->ClearLockOnTarget();

        ChangeCamera(FollowFromBehind());
        lockOnTarget_.reset();
        isLockedOn_ = false;
    }

    void PlayerAvatarCameraGroupBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("followFromBehindCameraName_", followFromBehindCameraName_);
        ImGuiHelper::OnDrawInputField("followFromBehindCamera_", followFromBehindCamera_);
        ImGuiHelper::OnDrawInputField("lockOnCameraName_", lockOnCameraName_);
    }
}
