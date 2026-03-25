#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../Follow/VirtualCameraFollowBehaviour.h"
#include "../LookAt/VirtualCameraLookAtBehaviour.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class ThirdPersonCameraBehaviour final
        : public Component::ComponentBase
        , public LifeCycleCallback::IAwakable
        , public LifeCycleCallback::IUpdatable
        , public IVirtualCameraBehaviour
    {
    public:
        void SetTarget(const std::shared_ptr<GameObject::IGameObject>& target);
        void SetLookAtOffsetPos(const glm::vec3& offsetPos);

    private:
        void OnAwake () override;
        void OnUpdate() override;
        void MainCameraCallback() override;

        void UpdateMouseInput();
        void UpdateGamepadInput();

        void UpdateFollowTargetBehaviour() const;
        void UpdateLookAtTargetBehaviour() const;

        
        float yaw_              = 0.0f;
        float pitch_            = 0.2f;
        float minPitch_         = -1.2f;
        float maxPitch_         =  1.2f;
        float mouseSensitivity_ = 0.005f;
        float distance_         = 5.0f;

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
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
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
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Component::ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
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
}
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(CineMachine::Behaviour::ThirdPersonCameraBehaviour, 3);
CEREAL_REGISTER_TYPE(CineMachine::Behaviour::ThirdPersonCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IAwakable, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IUpdatable, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(CineMachine::IVirtualCameraBehaviour, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
#pragma endregion
