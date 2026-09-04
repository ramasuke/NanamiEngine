#pragma once
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"
#include "Behaviour/IVirtualCameraBehaviour.h"

namespace NanamiEngine::CineMachine
{
    constexpr auto SAMPLE_CAMERA_FOV = 90.0f;
    constexpr auto DISABLE_PRIORITY = -1;
    
    class CineMachineVirtualCamera final : public Component::ComponentBase,
                                           public LifeCycleCallback::IAwakable,
                                           public LifeCycleCallback::IDebugRenderable
    {
    public:
        [[nodiscard]] Rx::ReadOnlyReactiveContext<int> Priority() const { return priority_.AsReadOnly(); }
        void SetPriority(int priority);
        
        // VirtualCameraの完全無効化
        void OnDisable() { priority_.OnNext(DISABLE_PRIORITY); }
        void MainCameraCallback() const;
        [[nodiscard]] bool WantsImmediateApply() const;

    private:
        void OnAwake      () override;
        void OnDestroy    () override;
        void OnDrawGui    () override;
        void OnDebugRender() override;
        
        Rx::SerializableSubject<int> priority_ = Rx::SerializableSubject(0);
        std::vector<std::weak_ptr<IVirtualCameraBehaviour>> cameraBehaviours_;
    
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(priority_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 0) archive(CEREAL_NVP(priority_));
}
#pragma endregion
};
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::CineMachineVirtualCamera, 0)
