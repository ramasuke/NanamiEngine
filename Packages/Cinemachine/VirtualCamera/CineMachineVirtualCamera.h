#pragma once
#include <optional>

#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Libs/LibCore/Tween/Ease/Type/EaseType.h"
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../R4/R4.h"
#include "Behaviour/IVirtualCameraBehaviour.h"

namespace NanamiEngine::CineMachine
{
    constexpr auto SAMPLE_CAMERA_FOV = 90.0f;
    constexpr auto DISABLE_PRIORITY = -1;

    // NOTE: このカメラへ切り替わるときの補間
    struct NANAMI_API BlendIn
    {
        float            duration_secs = 0.5f;
        LibCore::EaseType ease         = LibCore::EaseType::SmoothStep;
    };
    
    class NANAMI_API CineMachineVirtualCamera final : public Component::ComponentBase,
                                           public LifeCycleCallback::IAwakable,
                                           public LifeCycleCallback::IStartable,
                                           public LifeCycleCallback::IDebugRenderable
    {
    public:
        [[nodiscard]] R4::ReadOnlyReactiveProperty<int> Priority() const { return priority_.AsReadOnly(); }
        void SetPriority(int priority);
        // NOTE: このカメラで使うFOV(度)。上書きしていなければBrainの既定FOVを返す
        [[nodiscard]] float Fov() const;
        void SetImmediateApply(const bool enable) { isImmediateApply_ = enable; }
        // NOTE: このカメラへ切り替わるときの補間。上書きしていなければ nullopt
        [[nodiscard]] std::optional<BlendIn> CustomBlendIn() const;
        void SetBlendIn(float duration_secs, LibCore::EaseType ease);
        void ClearBlendIn() { overrideBlendIn_ = false; }

        void OnDisable() { priority_.Value(DISABLE_PRIORITY); }
        // NOTE: 毎フレーム呼ばれ、Behaviour を Stage() の順に更新する
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
        // NOTE: 既定の FOV ではなく、このカメラ独自の FOV を使うか
        bool  overrideFov_ = false;
        float fov_         = 60.0f;
        bool              overrideBlendIn_ = false;
        float             blendIn_secs_    = 0.5f;
        LibCore::EaseType blendInEase_     = LibCore::EaseType::SmoothStep;
        // NOTE: Behaviour に関係なく追従補間をスキップする (非シリアライズ)
        bool  isImmediateApply_ = false;
        std::vector<std::weak_ptr<IVirtualCameraBehaviour>> cameraBehaviours_;
    
#pragma region Serialization Function
public:
template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(priority_));
    archive(CEREAL_NVP(overrideFov_));
    archive(CEREAL_NVP(fov_));
    archive(CEREAL_NVP(overrideBlendIn_));
    archive(CEREAL_NVP(blendIn_secs_));
    archive(CEREAL_NVP(blendInEase_));
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
    if (version >= 2)
    {
        archive(CEREAL_NVP(overrideBlendIn_));
        archive(CEREAL_NVP(blendIn_secs_));
        archive(CEREAL_NVP(blendInEase_));
    }
}
#pragma endregion
};
}

CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::CineMachineVirtualCamera, 2);
