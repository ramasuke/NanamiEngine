#include "Prop_AirShipWingFlap.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void AirShipWingFlap::OnAwake()
    {
        initialLocalRot_ = Transform().GetLocalRot();
    }

    void AirShipWingFlap::OnUpdate()
    {
        if (!IsEnable())
            return;

        const float dt = Time::DeltaTime();
        elapsed_secs_ += dt;

        // NOTE: 止める / 動かすときに角度が跳ねないよう、振幅を 1 秒ほどかけて寄せる
        const float target = isFlapping_ ? amplitudeDeg_ : 0.0f;
        currentAmplitudeDeg_ += (target - currentAmplitudeDeg_) * glm::min(dt * 2.0f, 1.0f);

        const float period = glm::max(period_secs_, 0.01f);
        const float wave   = glm::sin(glm::two_pi<float>() * (elapsed_secs_ / period + phase01_));
        const float angle  = glm::radians(baseAngleDeg_ + currentAmplitudeDeg_ * wave);
        if (glm::length(rotateAxis_) < 1e-4f)
            return;

        Transform().SetLocalRot(initialLocalRot_ * glm::angleAxis(angle, glm::normalize(rotateAxis_)));
    }

    void AirShipWingFlap::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rotateAxis_",   rotateAxis_);
        ImGuiHelper::OnDrawInputField("baseAngleDeg_", baseAngleDeg_);
        ImGuiHelper::OnDrawInputField("amplitudeDeg_", amplitudeDeg_);
        ImGuiHelper::OnDrawInputField("period_secs_",  period_secs_);
        ImGuiHelper::OnDrawInputField("phase01_",      phase01_);
        ImGuiHelper::OnDrawInputField("isFlapping_",   isFlapping_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::AirShipWingFlap);
#pragma endregion
