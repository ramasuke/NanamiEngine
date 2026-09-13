#include "NoiseCameraBehaviour.h"

#include <algorithm>
#include "../../../../../Libs/glm/gtc/noise.hpp"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Brain/CinemachineCameraBrain.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    namespace
    {
        constexpr int   MAX_OCTAVES          = 8;
        constexpr float OCTAVE_SEED_STEP     = 17.31f;
        constexpr float ROTATION_SEED_OFFSET = 131.7f;
    }

    void NoiseCameraBehaviour::OnBecameLive()
    {
        // カメラが切り替わった瞬間に揺れが急に乗らないよう、0からフェードインさせる。
        weight_ = 0.0f;
    }

    float NoiseCameraBehaviour::FractalNoise(const float seed, const float t) const
    {
        const int octaves = std::clamp(octaves_, 1, MAX_OCTAVES);

        float value        = 0.0f;
        float amplitude    = 1.0f;
        float frequency    = 1.0f;
        float amplitudeSum = 0.0f;
        for (int i = 0; i < octaves; ++i)
        {
            value        += glm::perlin(glm::vec2(seed + OCTAVE_SEED_STEP * static_cast<float>(i), t * frequency)) * amplitude;
            amplitudeSum += amplitude;
            amplitude    *= 0.5f;
            frequency    *= 2.0f;
        }
        return value / amplitudeSum;
    }

    void NoiseCameraBehaviour::MainCameraCallback()
    {
        const float dt = Time::DeltaTime();
        time_ += dt * frequencyGain_;

        if (blendIn_secs_ > 0.0f)
            weight_ = std::min(1.0f, weight_ + dt / blendIn_secs_);
        else
            weight_ = 1.0f;

        const float gain = amplitudeGain_ * glm::smoothstep(0.0f, 1.0f, weight_);
        if (gain <= 0.0f)
            return;

        const float t = time_ * frequency_;

        const glm::vec3 posNoise(
            FractalNoise(seed_.x, t),
            FractalNoise(seed_.y, t),
            FractalNoise(seed_.z, t));
        const glm::vec3 rotNoise(
            FractalNoise(seed_.x + ROTATION_SEED_OFFSET, t),
            FractalNoise(seed_.y + ROTATION_SEED_OFFSET, t),
            FractalNoise(seed_.z + ROTATION_SEED_OFFSET, t));

        const glm::vec3 posOffset = gain * positionAmplitude_ * posNoise;
        const glm::vec3 angleRad  = gain * glm::radians(rotationAmplitude_deg_) * rotNoise;

        const glm::vec3 brainPos = CinemachineCameraBrain::Instance()->Transform().GetWorldPos();
        const glm::quat brainRot = CinemachineCameraBrain::Instance()->Transform().GetWorldRot();

        CinemachineCameraBrain::Instance()->Transform().SetWorldPos(brainPos + brainRot * posOffset);
        CinemachineCameraBrain::Instance()->Transform().SetWorldRot(brainRot * glm::quat(angleRad));
    }

    void NoiseCameraBehaviour::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rotationAmplitude_deg_", rotationAmplitude_deg_);
        ImGuiHelper::OnDrawInputField("positionAmplitude_",     positionAmplitude_);
        ImGuiHelper::OnDrawInputField("frequency_",             frequency_);
        ImGuiHelper::OnDrawInputField("octaves_",               octaves_);
        ImGuiHelper::OnDrawInputField("amplitudeGain_",         amplitudeGain_);
        ImGuiHelper::OnDrawInputField("frequencyGain_",         frequencyGain_);
        ImGuiHelper::OnDrawInputField("blendIn_secs_",          blendIn_secs_);
        ImGuiHelper::OnDrawInputField("seed_",                  seed_);
    }
}
