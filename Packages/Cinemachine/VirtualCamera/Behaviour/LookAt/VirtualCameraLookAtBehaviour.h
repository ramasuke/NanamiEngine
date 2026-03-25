#pragma once
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class VirtualCameraLookAtBehaviour final : public Component::ComponentBase,
                                               public LifeCycleCallback::IAwakable,
                                               public LifeCycleCallback::IUpdatable,
                                               public LifeCycleCallback::IDebugRenderable,
                                               public IVirtualCameraBehaviour
    {
    public:
        void SetTarget(const std::shared_ptr<GameObject::IGameObject>& target);
        void SetOffsetPos(glm::vec3 lookAtTargetOffset);
        
    private:
        void OnAwake      () override;
        void OnUpdate     () override;
        void OnDebugRender() override;
        void LookAtTarget () const;

        FIELD(Module::GameObject::IGameObject) target_;
        glm::vec3 lookAtTargetOffset_ = glm::vec3(0.0f);
    
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("target_", target_);
    LibCore::ImGuiHelper::OnDrawInputField("lookAtTargetOffset_", lookAtTargetOffset_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<IAwakable>(this));
    archive(cereal::base_class<IUpdatable>(this));
    archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    archive(CEREAL_NVP(target_));
    archive(CEREAL_NVP(lookAtTargetOffset_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<IAwakable>(this));
    archive(cereal::base_class<IUpdatable>(this));
    archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(target_));
    if (version >= 1) archive(CEREAL_NVP(lookAtTargetOffset_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour);
#pragma endregion
