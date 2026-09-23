#include "ShakeCameraBehaviour.h"

#include "gtc/noise.hpp"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Brain/CinemachineCameraBrain.h"
#include "../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    std::vector<ShakeCameraBehaviour*> ShakeCameraBehaviour::instances_;

    void ShakeCameraBehaviour::Shake(
        const float intensity,
        const float duration)
    {
        if (duration <= 0.0f)
            return;

        // NOTE: 重ねた揺れも 1 あたり duration 秒の速さで 0 へ減らす
        const float trauma = std::clamp(trauma_.Value() + intensity, 0.0f, 1.0f);
        trauma_.Play(tweeny::from(trauma).to(0.0f).during(LibCore::Tween::Ms(duration * trauma)));
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

        trauma_.Tick(deltaTime);
    }

    void ShakeCameraBehaviour::MainCameraCallback()
    {
        const float trauma = trauma_.Value();
        if (trauma <= 0.0f && sustain_ <= 0.0f)
            return;

        const float shake        = trauma * trauma;
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

        auto* brain = CinemachineCameraBrain::Instance();
        if (!brain)
            return;

        const glm::vec3 brainPos = brain->Transform().GetWorldPos();
        const glm::quat brainRot = brain->Transform().GetWorldRot();

        const glm::vec3 shakenPos = brainPos + brainRot * posOffset;
        const glm::quat shakenRot = brainRot * glm::quat(angleRad);

        brain->Transform().SetWorldPos(shakenPos);
        brain->Transform().SetWorldRot(shakenRot);
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(CineMachine::Behaviour::ShakeCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(CineMachine::IVirtualCameraBehaviour, CineMachine::Behaviour::ShakeCameraBehaviour);
#pragma endregion
