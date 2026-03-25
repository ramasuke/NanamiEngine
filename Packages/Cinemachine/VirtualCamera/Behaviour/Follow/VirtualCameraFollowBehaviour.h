#pragma once
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class VirtualCameraFollowBehaviour final : public Component::ComponentBase,
                                               public LifeCycleCallback::IUpdatable,
                                               public LifeCycleCallback::IDebugRenderable,
                                               public IVirtualCameraBehaviour
    {
    public:
        void SetTarget(const std::shared_ptr<GameObject::IGameObject>& followTarget);
        [[nodiscard]] glm::vec3 MoveTargetPosition() const noexcept; 

    private:
        void OnUpdate     () override;
        void OnDebugRender() override;

    public:
        FIELD(Module::GameObject::IGameObject) followTarget_;
        glm::vec3 followOffset_ = glm::vec3(0, 0, 0);
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("followTarget_", followTarget_);
    LibCore::ImGuiHelper::OnDrawInputField("followOffset_", followOffset_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<Module::Component::ComponentBase>(this));
    archive(cereal::base_class<Module::LifeCycleCallback::IUpdatable>(this));
    archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    archive(CEREAL_NVP(followTarget_));
    archive(CEREAL_NVP(followOffset_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Module::Component::ComponentBase>(this));
    archive(cereal::base_class<Module::LifeCycleCallback::IUpdatable>(this));
    if (version >= 2) archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(followTarget_));
    if (version >= 1) archive(CEREAL_NVP(followOffset_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour, 2);
CEREAL_REGISTER_TYPE(NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour);
#pragma endregion
