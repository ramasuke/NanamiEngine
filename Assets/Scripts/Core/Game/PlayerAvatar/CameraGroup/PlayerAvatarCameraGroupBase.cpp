#include "PlayerAvatarCameraGroupBase.h"

#include "Packages/Cinemachine/VirtualCamera/Behaviour/LockOn/LockOnCameraBehaviour.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar
{
    std::weak_ptr<CineMachine::CineMachineVirtualCamera> PlayerAvatarCameraGroupBase::FollowFromBehind() const
    {
        return followFromBehindCamera_.get();
    }

    std::weak_ptr<CineMachine::CineMachineVirtualCamera> PlayerAvatarCameraGroupBase::LockOnCamera() const
    {
        return lockOnCamera_.get();
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
        auto& followCamera = followFromBehindCamera_->Components();
        auto weakCamera =  followCamera.Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>();
        auto camera = weakCamera.lock();
        camera->SetTarget(playerAvatarObject);
        currentCamera_ = followFromBehindCamera_.get();

        auto shakeCamera = followCamera.Catch<CineMachine::Behaviour::ShakeCameraBehaviour>().lock();
        lockOnCamera_->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->SetFollowTarget(playerAvatarObject);
        // ロックオン開始まではカメラ優先度を最低にしておき、FollowFromBehind の妨げにならないようにする
        lockOnCamera_->OnDisable();
    }

    void PlayerAvatarCameraGroupBase::EngageLockOn(const std::shared_ptr<GameObject::IGameObject>& target,
                                                   const std::shared_ptr<GameObject::IGameObject>& part)
    {
        if (!target || !lockOnCamera_)
            return;

        lockOnCamera_->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->SetLockOnTarget(target, part ? part : target);
        // ロック中の切り替えではカメラはそのまま
        if (!isLockedOn_)
            ChangeCamera(LockOnCamera());
        lockOnTarget_ = target;
        lockOnPart_ = part;
        isLockedOn_ = true;
    }

    std::shared_ptr<GameObject::IGameObject> PlayerAvatarCameraGroupBase::LockOnAim() const
    {
        auto target = lockOnTarget_.lock();
        if (!target)
            return nullptr;
        if (auto part = lockOnPart_.lock())
            return part;
        return target;
    }

    void PlayerAvatarCameraGroupBase::ReleaseLockOn()
    {
        if (!isLockedOn_)
            return;

        if (lockOnCamera_)
            lockOnCamera_->Components().Catch<CineMachine::Behaviour::LockOnCameraBehaviour>().lock()->ClearLockOnTarget();

        ChangeCamera(FollowFromBehind());
        lockOnTarget_.reset();
        lockOnPart_.reset();
        isLockedOn_ = false;
    }

    void PlayerAvatarCameraGroupBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("followFromBehindCamera_", followFromBehindCamera_);
        ImGuiHelper::OnDrawInputField("lockOnCamera_", lockOnCamera_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase);
#pragma endregion
