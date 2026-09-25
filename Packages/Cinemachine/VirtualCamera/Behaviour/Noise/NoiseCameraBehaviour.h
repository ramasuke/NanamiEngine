#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    // 手持ちカメラのような微小な揺れ(Cinemachine の Basic Multi Channel Perlin 相当)を常時加えるビヘイビア。
    // 演出用VirtualCameraに付けると「止まっているのに自然に動いている」カメラになる。
    // MainCameraCallbackはアクティブなVirtualCameraでしか呼ばれないため、揺れるのはそのカメラが使われている間だけ。
    class NANAMI_API NoiseCameraBehaviour final : public Component::ComponentBase,
                                       public IVirtualCameraBehaviour
    {
    public:
        void SetAmplitudeGain(float amplitudeGain) { amplitudeGain_ = amplitudeGain; }
        [[nodiscard]] float AmplitudeGain() const { return amplitudeGain_; }

    private:
        void MainCameraCallback() override;
        void OnBecameLive() override;
        [[nodiscard]] float FractalNoise(float seed, float t) const;

        glm::vec3 rotationAmplitude_deg_ = glm::vec3(0.5f, 0.7f, 0.25f); // pitch / yaw / roll
        glm::vec3 positionAmplitude_     = glm::vec3(0.015f, 0.015f, 0.0f);
        float     frequency_             = 0.3f;
        int       octaves_               = 3;
        float     amplitudeGain_         = 1.0f;
        float     frequencyGain_         = 1.0f;
        float     blendIn_secs_          = 0.6f;
        glm::vec3 seed_                  = glm::vec3(27.13f, 53.71f, 89.37f);

        float time_   = 0.0f;
        float weight_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            archive(CEREAL_NVP(rotationAmplitude_deg_));
            archive(CEREAL_NVP(positionAmplitude_));
            archive(CEREAL_NVP(frequency_));
            archive(CEREAL_NVP(octaves_));
            archive(CEREAL_NVP(amplitudeGain_));
            archive(CEREAL_NVP(frequencyGain_));
            archive(CEREAL_NVP(blendIn_secs_));
            archive(CEREAL_NVP(seed_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            if (version >= 0) archive(CEREAL_NVP(rotationAmplitude_deg_));
            if (version >= 0) archive(CEREAL_NVP(positionAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(frequency_));
            if (version >= 0) archive(CEREAL_NVP(octaves_));
            if (version >= 0) archive(CEREAL_NVP(amplitudeGain_));
            if (version >= 0) archive(CEREAL_NVP(frequencyGain_));
            if (version >= 0) archive(CEREAL_NVP(blendIn_secs_));
            if (version >= 0) archive(CEREAL_NVP(seed_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(CineMachine::Behaviour::NoiseCameraBehaviour, 0);
