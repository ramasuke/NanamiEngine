#pragma once
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class VirtualCameraLookAtBehaviour final : public Component::ComponentBase,
                                               public LifeCycleCallback::IAwakable,
                                               public LifeCycleCallback::IDebugRenderable,
                                               public IVirtualCameraBehaviour
    {
    public:
        void SetTarget(const std::shared_ptr<GameObject::IGameObject>& target);
        [[nodiscard]] bool HasTarget() const { return static_cast<bool>(target_); }
        void SetOffsetPos(glm::vec3 lookAtTargetOffset);
        /** @brief 次のOnUpdateを待たずに、今の位置からtargetへ向ける */
        void LookAtTarget() const;

    private:
        void OnAwake       () override;
        void OnCameraUpdate() override;
        [[nodiscard]] VirtualCameraStage Stage() const override { return VirtualCameraStage::Aim; }
        void OnDebugRender () override;

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
    archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    archive(CEREAL_NVP(target_));
    archive(CEREAL_NVP(lookAtTargetOffset_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<IAwakable>(this));
    if (version <= 1) Module::LifeCycleCallback::DiscardUpdatableBase(archive);
    archive(cereal::base_class<IDebugRenderable>(this));
    archive(cereal::base_class<IVirtualCameraBehaviour>(this));
    if (version >= 0) archive(CEREAL_NVP(target_));
    if (version >= 1) archive(CEREAL_NVP(lookAtTargetOffset_));
}
#pragma endregion
};
}

CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour, 2);
