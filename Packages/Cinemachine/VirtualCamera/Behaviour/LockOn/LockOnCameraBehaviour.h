#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../Follow/VirtualCameraFollowBehaviour.h"
#include "../LookAt/VirtualCameraLookAtBehaviour.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class LockOnCameraBehaviour final
        : public Component::ComponentBase
        , public LifeCycleCallback::IAwakable
        , public LifeCycleCallback::IUpdatable
        , public IVirtualCameraBehaviour
    {
    public:
        void SetFollowTarget(const std::shared_ptr<GameObject::IGameObject>& followTarget);
        void SetLockOnTarget(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget);
        void ClearLockOnTarget();

    private:
        bool WantsImmediateApply() const override { return isImmediateApply_; }

        void OnAwake () override;
        void OnUpdate() override;
        int  UpdatePriority() const override { return 1; }

        void UpdateFollowBehaviour(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget) const;
        // Playerからカメラへrayを飛ばし、障害物にめり込まない位置までオフセットを縮める
        [[nodiscard]] glm::vec3 ResolveCameraCollision(const glm::vec3& originPos, const glm::vec3& desiredOffset) const;

        // ブレインの補完を無視して仮想カメラのTransformを即時適用するか。
        // ロックオン開始の瞬間はブレインの補間で滑らかに寄せたいため既定で無効。
        bool isImmediateApply_ = false;

        float distance_           = 9.0f;
        float height_             = 3.5f;
        // プレイヤーとロックオン対象を両方フレームに収めるための横オフセット
        float sideOffset_         = 1.5f;
        float lookAtHeightOffset_ = 1.2f;
        // 障害物にめり込まないようカメラを手前に寄せる際の余白
        float collisionBuffer_    = 0.3f;

        FIELD(GameObject::IGameObject                ) followTarget_;
        FIELD(Behaviour::VirtualCameraFollowBehaviour) follow_;
        FIELD(Behaviour::VirtualCameraLookAtBehaviour) lookAt_;

        // ロックオン対象はON/OFFの度に頻繁に空へ戻す必要があるが、
        // FIELD<T>::set(nullptr) は no-op のため FIELD では解除できない。
        // そのため素の weak_ptr で保持する(エディタからのドラッグ&ドロップ割り当ては非対応、
        // 実行時に SetLockOnTarget/ClearLockOnTarget で注入する前提)。
        std::weak_ptr<GameObject::IGameObject> lockOnTarget_;

#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    archive(CEREAL_NVP(isImmediateApply_));
    archive(CEREAL_NVP(distance_));
    archive(CEREAL_NVP(height_));
    archive(CEREAL_NVP(sideOffset_));
    archive(CEREAL_NVP(lookAtHeightOffset_));
    archive(CEREAL_NVP(collisionBuffer_));
    archive(CEREAL_NVP(followTarget_));
    archive(CEREAL_NVP(follow_));
    archive(CEREAL_NVP(lookAt_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(isImmediateApply_));
    if (version >= 0) archive(CEREAL_NVP(distance_));
    if (version >= 0) archive(CEREAL_NVP(height_));
    if (version >= 0) archive(CEREAL_NVP(sideOffset_));
    if (version >= 0) archive(CEREAL_NVP(lookAtHeightOffset_));
    if (version >= 0) archive(CEREAL_NVP(collisionBuffer_));
    if (version >= 0) archive(CEREAL_NVP(followTarget_));
    if (version >= 0) archive(CEREAL_NVP(follow_));
    if (version >= 0) archive(CEREAL_NVP(lookAt_));
}
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour, 0)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
