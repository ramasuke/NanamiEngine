#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../CineMachineVirtualCamera.h"
#include "../Follow/VirtualCameraFollowBehaviour.h"
#include "../LookAt/VirtualCameraLookAtBehaviour.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class NANAMI_API LockOnCameraBehaviour final
        : public Component::ComponentBase
        , public LifeCycleCallback::IAwakable
        , public IVirtualCameraBehaviour
    {
    public:
        void SetFollowTarget(const std::shared_ptr<GameObject::IGameObject>& followTarget);
        // 向きと画角は lockOnTarget で決め、aim(部位など)はフレーミングに加えるだけ
        void SetLockOnTarget(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget,
                             const std::shared_ptr<GameObject::IGameObject>& aim);
        void ClearLockOnTarget();

    private:
        bool WantsImmediateApply() const override { return isImmediateApply_; }

        void OnAwake       () override;
        void OnCameraUpdate() override;
        // Follow/LookAtが読むオフセットを先に書き込む
        [[nodiscard]] VirtualCameraStage Stage() const override { return VirtualCameraStage::Driver; }
        
        void UpdateFraming(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget);
        [[nodiscard]] glm::vec3 ResolveCameraCollision(const glm::vec3& originPos, const glm::vec3& desiredOffset) const;
        
        bool isImmediateApply_ = false;
        
        float pitchAngle_deg_       = 15.0f;
        float minDistance_          = 8.0f;
        float maxDistance_          = 45.0f;
        float framingMargin_        = 0.1f;
        float fallbackBoundsRadius_ = 1.0f;
        float collisionBuffer_      = 0.3f;
        float collisionRadius_      = 2.0f;
        
        glm::vec3 lastFlatDir_ = glm::vec3(0.0f, 0.0f, 1.0f);

        FIELD(GameObject::IGameObject                ) followTarget_;
        FIELD(Behaviour::VirtualCameraFollowBehaviour) follow_;
        FIELD(Behaviour::VirtualCameraLookAtBehaviour) lookAt_;
        
        std::weak_ptr<GameObject::IGameObject> lockOnTarget_;
        std::weak_ptr<GameObject::IGameObject> lockOnAim_;
        std::weak_ptr<CineMachineVirtualCamera> virtualCamera_;

#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
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
    if (version <= 2) LifeCycleCallback::DiscardUpdatableBase(archive);
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(isImmediateApply_));
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

CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour, 3);
