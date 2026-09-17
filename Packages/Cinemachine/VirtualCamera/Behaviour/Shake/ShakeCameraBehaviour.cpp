#include "ShakeCameraBehaviour.h"

#include "gtc/noise.hpp"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Brain/CinemachineCameraBrain.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    std::vector<ShakeCameraBehaviour*> ShakeCameraBehaviour::instances_;

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
        for (auto* instance : instances_)
            instance->Shake(intensity, duration);
    }

    void ShakeCameraBehaviour::ShakeMainCamera()
    {
        for (auto* instance : instances_)
            instance->Shake();
    }

    void ShakeCameraBehaviour::SustainShake(const float intensity)
    {
        sustainRequest_ = std::max(sustainRequest_, std::clamp(intensity, 0.0f, 1.0f));
    }

    void ShakeCameraBehaviour::SustainShakeMainCamera(const float intensity)
    {
        for (auto* instance : instances_)
            instance->SustainShake(intensity);
    }

    void ShakeCameraBehaviour::OnAwake()
    {
        instances_.push_back(this);
    }

    void ShakeCameraBehaviour::OnDestroy()
    {
        std::erase(instances_, this);
    }

    void ShakeCameraBehaviour::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        // 要求は毎フレーム消費する。呼び出し側が要求をやめれば(ステート離脱・破棄を含む)自然に0へ戻る
        const float blend = 1.0f - std::exp(-deltaTime / std::max(sustainSmoothTime_secs_, 0.001f));
        sustain_ += (sustainRequest_ - sustain_) * blend;
        if (sustainRequest_ <= 0.0f && sustain_ < 0.001f)
            sustain_ = 0.0f;
        sustainRequest_ = 0.0f;

        if (trauma_ <= 0.0f)
            return;

        trauma_ = std::max(0.0f, trauma_ - deltaTime / duration_);
    }

    void ShakeCameraBehaviour::MainCameraCallback()
    {
        if (trauma_ <= 0.0f && sustain_ <= 0.0f)
            return;

        const float shake        = trauma_ * trauma_;
        const float sustainShake = sustain_ * sustain_;
        const float t            = Time::CurrentTime() * frequency_;
        const float sustainT     = Time::CurrentTime() * sustainFrequency_;

        const glm::vec3 posNoise(
            glm::perlin(glm::vec2(seed_.x,         t)),
            glm::perlin(glm::vec2(seed_.y,         t)),
            glm::perlin(glm::vec2(seed_.z,         t)));
        const glm::vec3 rotNoise(
            glm::perlin(glm::vec2(seed_.x + 101.0f, t)),
            glm::perlin(glm::vec2(seed_.y + 211.0f, t)),
            glm::perlin(glm::vec2(seed_.z + 307.0f, t)));
        const glm::vec3 sustainPosNoise(
            glm::perlin(glm::vec2(seed_.x + 401.0f, sustainT)),
            glm::perlin(glm::vec2(seed_.y + 503.0f, sustainT)),
            glm::perlin(glm::vec2(seed_.z + 601.0f, sustainT)));
        const glm::vec3 sustainRotNoise(
            glm::perlin(glm::vec2(seed_.x + 701.0f, sustainT)),
            glm::perlin(glm::vec2(seed_.y + 809.0f, sustainT)),
            glm::perlin(glm::vec2(seed_.z + 907.0f, sustainT)));

        const glm::vec3 posOffset = posAmplitude_ * (shake * posNoise + sustainShake * sustainPosNoise);
        const glm::vec3 angleRad  = glm::radians(angleAmplitude_) * (shake * rotNoise + sustainShake * sustainRotNoise);

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
        ImGuiHelper::OnDrawInputField("sustainFrequency_",       sustainFrequency_);
        ImGuiHelper::OnDrawInputField("sustainSmoothTime_secs_", sustainSmoothTime_secs_);

        if (ImGui::Button("Test Shake"))
            Shake();

        ImGui::Button("Test Sustain (Hold)");
        if (ImGui::IsItemActive())
            SustainShake(defaultIntensity_);
    }
}
