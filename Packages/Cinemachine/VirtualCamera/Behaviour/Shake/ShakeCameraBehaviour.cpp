#include "ShakeCameraBehaviour.h"

#include "gtc/noise.hpp"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Brain/CinemachineCameraBrain.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    ShakeCameraBehaviour* ShakeCameraBehaviour::instance_ = nullptr;

    void ShakeCameraBehaviour::Shake(
        const float intensity,
        const float duration)
    {
        if (duration <= 0.0f)
            return;

        trauma_   = std::clamp(trauma_ + intensity, 0.0f, 1.0f);
        duration_ = duration;
    }

    void ShakeCameraBehaviour::Shake()
    {
        Shake(defaultIntensity_, defaultDuration_);
    }

    void ShakeCameraBehaviour::ShakeMainCamera(const float intensity, const float duration)
    {
        instance_->Shake(intensity, duration);
    }

    void ShakeCameraBehaviour::ShakeMainCamera()
    {
        instance_->Shake();
    }

    void ShakeCameraBehaviour::OnAwake()
    {
        instance_ = this;
    }

    void ShakeCameraBehaviour::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }

    void ShakeCameraBehaviour::OnUpdate()
    {
        if (trauma_ <= 0.0f)
            return;

        trauma_ = std::max(0.0f, trauma_ - Time::DeltaTime() / duration_);
    }

    void ShakeCameraBehaviour::MainCameraCallback()
    {
        if (trauma_ <= 0.0f)
            return;

        const float shake = trauma_ * trauma_;
        const float t     = Time::CurrentTime() * frequency_;

        const glm::vec3 posNoise(
            glm::perlin(glm::vec2(seed_.x,         t)),
            glm::perlin(glm::vec2(seed_.y,         t)),
            glm::perlin(glm::vec2(seed_.z,         t)));
        const glm::vec3 rotNoise(
            glm::perlin(glm::vec2(seed_.x + 101.0f, t)),
            glm::perlin(glm::vec2(seed_.y + 211.0f, t)),
            glm::perlin(glm::vec2(seed_.z + 307.0f, t)));

        const glm::vec3 posOffset = shake * posAmplitude_ * posNoise;
        const glm::vec3 angleRad  = shake * glm::radians(angleAmplitude_) * rotNoise;

        const glm::vec3 brainPos = CinemachineCameraBrain::Instance()->Transform().GetWorldPos();
        const glm::quat brainRot = CinemachineCameraBrain::Instance()->Transform().GetWorldRot();

        const glm::vec3 shakenPos = brainPos + brainRot * posOffset;
        const glm::quat shakenRot = brainRot * glm::quat(angleRad);

        CinemachineCameraBrain::Instance()->Transform().SetWorldPos(shakenPos);
        CinemachineCameraBrain::Instance()->Transform().SetWorldRot(shakenRot);
    }

    void ShakeCameraBehaviour::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("posAmplitude_",     posAmplitude_);
        ImGuiHelper::OnDrawInputField("angleAmplitude_",   angleAmplitude_);
        ImGuiHelper::OnDrawInputField("frequency_",        frequency_);
        ImGuiHelper::OnDrawInputField("defaultIntensity_", defaultIntensity_);
        ImGuiHelper::OnDrawInputField("defaultDuration_",  defaultDuration_);
        ImGuiHelper::OnDrawInputField("seed_",             seed_);

        if (ImGui::Button("Test Shake"))
            Shake();
    }
}
