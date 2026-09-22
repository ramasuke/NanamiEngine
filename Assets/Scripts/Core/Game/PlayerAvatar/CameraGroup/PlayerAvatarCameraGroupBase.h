#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GameCore::PlayerAvatar
{
    constexpr auto ENABLE_CURRENT_CAMERA_PRIORITY = 0;
    
    class PlayerAvatarCameraGroupBase : public Component::ComponentBase
    {
    public:
        virtual ~PlayerAvatarCameraGroupBase() override = default;
        [[nodiscard]] std::weak_ptr<CineMachine::CineMachineVirtualCamera> FollowFromBehind() const;
        [[nodiscard]] std::weak_ptr<CineMachine::CineMachineVirtualCamera> LockOnCamera() const;
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera);
        virtual void Init(const std::shared_ptr<GameObject::IGameObject>& playerAvatarObject);
        [[nodiscard]] CineMachine::CineMachineVirtualCamera& CurrentCamera() const { return *currentCamera_.lock(); }

        // 敵をロックオンしてカメラを切り替える。target が nullptr なら何もしない。part が nullptr なら本体を狙う
        void EngageLockOn(const std::shared_ptr<GameObject::IGameObject>& target, const std::shared_ptr<GameObject::IGameObject>& part);
        // ロックオンを解除し、FollowFromBehind カメラへ戻す
        void ReleaseLockOn();
        [[nodiscard]] bool IsLockedOn() const { return isLockedOn_; }
        // ロック中の本体。部位を狙っていても本体を返す
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> LockOnTarget() const { return lockOnTarget_; }
        // 本体を狙っている間は空
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> LockOnPart() const { return lockOnPart_; }
        // 狙っている先。部位が残っていれば部位、それ以外は本体
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> LockOnAim() const;

        void SetLockOnCandidate(const std::shared_ptr<GameObject::IGameObject>& candidate) { lockOnCandidate_ = candidate; }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> LockOnCandidate() const { return lockOnCandidate_; }

    private:
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) followFromBehindCamera_;
        [[serialize(3)]] FIELD(CineMachine::CineMachineVirtualCamera) lockOnCamera_;

        std::weak_ptr<CineMachine::CineMachineVirtualCamera> currentCamera_;

        std::weak_ptr<GameObject::IGameObject> lockOnTarget_;
        std::weak_ptr<GameObject::IGameObject> lockOnPart_;
        std::weak_ptr<GameObject::IGameObject> lockOnCandidate_;
        bool isLockedOn_ = false;

#pragma region Serialization Function
public:
void BasedOnDrawgui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(followFromBehindCamera_));
    archive(CEREAL_NVP(lockOnCamera_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 0) archive(CEREAL_NVP(followFromBehindCamera_));
    if (version >= 3) archive(CEREAL_NVP(lockOnCamera_));
}
#pragma endregion
};
}

ENGINE_REGISTER_COMPONENT(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase, 3)
