#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../Follow/VirtualCameraFollowBehaviour.h"
#include "../LookAt/VirtualCameraLookAtBehaviour.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class NANAMI_API ThirdPersonCameraBehaviour final
        : public Component::ComponentBase
        , public LifeCycleCallback::IAwakable
        , public IVirtualCameraBehaviour
    {
    public:
        void SetTarget(const std::shared_ptr<GameObject::IGameObject>& target);
        void SetLookAtOffsetPos(const glm::vec3& offsetPos);
        void SetEnableLockMousePos(bool enable);
        void SetEnableImmediateApply(bool enable);

        [[nodiscard]] static bool IsMousePinned();

    private:
        bool WantsImmediateApply() const override { return isImmediateApply_; }

        void OnAwake       () override;
        void OnCameraUpdate() override;
        // Follow/LookAtが読むオフセットを先に書き込む
        [[nodiscard]] VirtualCameraStage Stage() const override { return VirtualCameraStage::Driver; }

        void UpdateMouseInput();
        void UpdateGamepadInput();

        void UpdateFollowTargetBehaviour() const;
        void UpdateLookAtTargetBehaviour() const;

        // Playerからカメラへrayを飛ばし、障害物にめり込まない位置までオフセットを縮める
        [[nodiscard]] glm::vec3 ResolveCameraCollision(const glm::vec3& desiredOffset) const;

        // NOTE: マウスカーソルは1つなので、どのインスタンスが固定したかは問わず全体で共有する(IsMousePinned 用)
        static int lastMousePinnedMs_;

        bool isLockMousePos_ = true;
        bool isImmediateApply_ = true;
        bool isMouseDeltaStale_ = true;

        float yaw_              = 0.0f;
        float pitch_            = -0.3f;
        float minPitch_         = -1.2f;
        float maxPitch_         =  1.2f;
        float mouseSensitivity_ = 0.005f;
        float distance_         = 5.0f;
        // 障害物にめり込まないようカメラを手前に寄せる際の余白
        float collisionBuffer_  = 0.3f;
        // めり込み判定に使う球の半径。カメラ周囲に確保する最低限の空き
        float collisionRadius_  = 2.0f;

        FIELD(GameObject::IGameObject                ) cameraBrain_;
        FIELD(GameObject::IGameObject                ) target_;
        FIELD(Behaviour::VirtualCameraFollowBehaviour) follow_;
        FIELD(Behaviour::VirtualCameraLookAtBehaviour) lookAt_;

        // Offsets
        glm::vec3 followOffsetPos_ = glm::vec3(0.0f);
        glm::vec3 lookAtOffsetPos_ = glm::vec3(0.0f, 1.0f, 0.0f);

#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    archive(CEREAL_NVP(minPitch_));
    archive(CEREAL_NVP(maxPitch_));
    archive(CEREAL_NVP(mouseSensitivity_));
    archive(CEREAL_NVP(distance_));
    archive(CEREAL_NVP(target_));
    archive(CEREAL_NVP(follow_));
    archive(CEREAL_NVP(lookAt_));
    archive(CEREAL_NVP(followOffsetPos_));
    archive(CEREAL_NVP(lookAtOffsetPos_));
    archive(CEREAL_NVP(cameraBrain_));
    archive(CEREAL_NVP(isImmediateApply_));
    archive(CEREAL_NVP(collisionBuffer_));
    archive(CEREAL_NVP(collisionRadius_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    if (version <= 6) LifeCycleCallback::DiscardUpdatableBase(archive);
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(minPitch_));
    if (version >= 0) archive(CEREAL_NVP(maxPitch_));
    if (version >= 0) archive(CEREAL_NVP(mouseSensitivity_));
    if (version >= 0) archive(CEREAL_NVP(distance_));
    if (version >= 0) archive(CEREAL_NVP(target_));
    if (version >= 0) archive(CEREAL_NVP(follow_));
    if (version >= 0) archive(CEREAL_NVP(lookAt_));
    if (version >= 1) archive(CEREAL_NVP(followOffsetPos_));
    if (version >= 2) archive(CEREAL_NVP(lookAtOffsetPos_));
    if (version >= 3) archive(CEREAL_NVP(cameraBrain_));
    if (version >= 4) archive(CEREAL_NVP(isImmediateApply_));
    if (version >= 5) archive(CEREAL_NVP(collisionBuffer_));
    if (version >= 6) archive(CEREAL_NVP(collisionRadius_));
}
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(CineMachine::Behaviour::ThirdPersonCameraBehaviour, 7);
