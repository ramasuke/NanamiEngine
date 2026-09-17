#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../CineMachineVirtualCamera.h"
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

        // プレイヤーとロックオン対象のバウンディングボックスが両方画角に収まる距離・注視点を求めて配置する
        void UpdateFraming(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget);
        // Playerからカメラへrayを飛ばし、障害物にめり込まない位置までオフセットを縮める
        [[nodiscard]] glm::vec3 ResolveCameraCollision(const glm::vec3& originPos, const glm::vec3& desiredOffset) const;

        // ブレインの補完を無視して仮想カメラのTransformを即時適用するか。
        // ロックオン開始の瞬間はブレインの補間で滑らかに寄せたいため既定で無効。
        bool isImmediateApply_ = false;

        // プレイヤー→ロックオン対象の方向から見下ろす角度
        float pitchAngle_deg_       = 15.0f;
        float minDistance_          = 8.0f;
        float maxDistance_          = 45.0f;
        // 画面端に残す余白の割合(0～1)
        float framingMargin_        = 0.1f;
        // コライダーを持たない対象を、位置を中心にこの半径の立方体とみなしてフレームに収める
        float fallbackBoundsRadius_ = 1.0f;
        // 障害物にめり込まないようカメラを手前に寄せる際の余白
        float collisionBuffer_      = 0.3f;
        // めり込み判定に使う球の半径。カメラ周囲に確保する最低限の空き
        float collisionRadius_      = 2.0f;

        // プレイヤーとロックオン対象が重なって方向が決まらないときは直前の方向を使い、カメラが急に回らないようにする
        glm::vec3 lastFlatDir_ = glm::vec3(0.0f, 0.0f, 1.0f);

        FIELD(GameObject::IGameObject                ) followTarget_;
        FIELD(Behaviour::VirtualCameraFollowBehaviour) follow_;
        FIELD(Behaviour::VirtualCameraLookAtBehaviour) lookAt_;

        // ロックオン対象はON/OFFの度に頻繁に空へ戻す必要があるが、
        // FIELD<T>::set(nullptr) は no-op のため FIELD では解除できない。
        // そのため素の weak_ptr で保持する(エディタからのドラッグ&ドロップ割り当ては非対応、
        // 実行時に SetLockOnTarget/ClearLockOnTarget で注入する前提)。
        std::weak_ptr<GameObject::IGameObject> lockOnTarget_;

        // フレーミングは自分が乗っているVirtualCameraの画角で解くため保持する(非シリアライズ)
        std::weak_ptr<CineMachineVirtualCamera> virtualCamera_;

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
    archive(CEREAL_NVP(collisionBuffer_));
    archive(CEREAL_NVP(followTarget_));
    archive(CEREAL_NVP(follow_));
    archive(CEREAL_NVP(lookAt_));
    archive(CEREAL_NVP(collisionRadius_));
    archive(CEREAL_NVP(pitchAngle_deg_));
    archive(CEREAL_NVP(minDistance_));
    archive(CEREAL_NVP(maxDistance_));
    archive(CEREAL_NVP(framingMargin_));
    archive(CEREAL_NVP(fallbackBoundsRadius_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(isImmediateApply_));
    // version 1 までの固定オフセット方式のパラメータ(フレーミング方式に移行)は読み捨てる
    if (version <= 1)
    {
        float legacyDistance = 0.0f;
        float legacyHeight = 0.0f;
        float legacySideOffset = 0.0f;
        float legacyLookAtHeightOffset = 0.0f;
        archive(cereal::make_nvp("distance_", legacyDistance));
        archive(cereal::make_nvp("height_", legacyHeight));
        archive(cereal::make_nvp("sideOffset_", legacySideOffset));
        archive(cereal::make_nvp("lookAtHeightOffset_", legacyLookAtHeightOffset));
    }
    if (version >= 0) archive(CEREAL_NVP(collisionBuffer_));
    if (version >= 0) archive(CEREAL_NVP(followTarget_));
    if (version >= 0) archive(CEREAL_NVP(follow_));
    if (version >= 0) archive(CEREAL_NVP(lookAt_));
    if (version >= 1) archive(CEREAL_NVP(collisionRadius_));
    if (version >= 2) archive(CEREAL_NVP(pitchAngle_deg_));
    if (version >= 2) archive(CEREAL_NVP(minDistance_));
    if (version >= 2) archive(CEREAL_NVP(maxDistance_));
    if (version >= 2) archive(CEREAL_NVP(framingMargin_));
    if (version >= 2) archive(CEREAL_NVP(fallbackBoundsRadius_));
}
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour, 2)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
