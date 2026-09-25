#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../R4/R4.h"
#include "Behaviour/IVirtualCameraBehaviour.h"

namespace NanamiEngine::CineMachine
{
    constexpr auto SAMPLE_CAMERA_FOV = 90.0f;
    constexpr auto DISABLE_PRIORITY = -1;
    
    class NANAMI_API CineMachineVirtualCamera final : public Component::ComponentBase,
                                           public LifeCycleCallback::IAwakable,
                                           public LifeCycleCallback::IStartable,
                                           public LifeCycleCallback::IDebugRenderable
    {
    public:
        [[nodiscard]] R4::ReadOnlyReactiveProperty<int> Priority() const { return priority_.AsReadOnly(); }
        void SetPriority(int priority);
        // このカメラで使うFOV(度)。上書きしていなければBrainの既定FOVを返す
        [[nodiscard]] float Fov() const;
        
        void OnDisable() { priority_.Value(DISABLE_PRIORITY); }
        // BrainがLateUpdateで毎フレーム呼ぶ。BehaviourをStage()の順に更新する
        void UpdateBehaviours() const;
        void MainCameraCallback() const;
        void OnBecameLive() const;
        [[nodiscard]] bool WantsImmediateApply() const;

    private:
        void OnAwake      () override;
        void OnStart      () override;
        void OnDestroy    () override;
        void OnDrawGui    () override;
        void OnDebugRender() override;
        
        R4::SerializableReactiveProperty<int> priority_ = R4::SerializableReactiveProperty(0);
        // Brainの既定FOVではなく、このカメラ独自のFOVを使うか
        bool  overrideFov_ = false;
        float fov_         = 60.0f;
        std::vector<std::weak_ptr<IVirtualCameraBehaviour>> cameraBehaviours_;
    
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(priority_));
    archive(CEREAL_NVP(overrideFov_));
    archive(CEREAL_NVP(fov_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 0) archive(CEREAL_NVP(priority_));
    if (version >= 1)
    {
        archive(CEREAL_NVP(overrideFov_));
        archive(CEREAL_NVP(fov_));
    }
}
#pragma endregion
};
}

CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::CineMachineVirtualCamera, 1);
