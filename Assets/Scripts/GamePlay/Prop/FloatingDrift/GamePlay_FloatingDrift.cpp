#include "GamePlay_FloatingDrift.h"

#include <cmath>
#include "gtc/constants.hpp"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void FloatingDrift::OnAwake()
    {
        homePos_ = Transform().GetLocalPos();
        homeRot_ = Transform().GetLocalRot();
        // NOTE: 置き場所から位相を決める。乱数だと再生のたびに見え方が変わる
        phase_ = std::fmod(std::abs(homePos_.x * 0.013f + homePos_.z * 0.007f), 1.0f) * glm::two_pi<float>();
    }

    void FloatingDrift::OnUpdate()
    {
        time_secs_ += Time::DeltaTime();

        const float bob = bobPeriod_secs_ > 0.0f
            ? std::sin(time_secs_ / bobPeriod_secs_ * glm::two_pi<float>() + phase_) * bobHeight_ : 0.0f;
        Transform().SetLocalPos(homePos_ + glm::vec3(0.0f, bob, 0.0f));

        if (tiltPeriod_secs_ <= 0.0f || tiltDegrees_ == 0.0f)
            return;

        const float angle = time_secs_ / tiltPeriod_secs_ * glm::two_pi<float>() + phase_;
        const float tilt  = glm::radians(tiltDegrees_);
        const glm::quat sway = glm::angleAxis(std::sin(angle) * tilt,        glm::vec3(1.0f, 0.0f, 0.0f))
                             * glm::angleAxis(std::cos(angle * 0.7f) * tilt, glm::vec3(0.0f, 0.0f, 1.0f));
        Transform().SetLocalRot(sway * homeRot_);
    }

    void FloatingDrift::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bobHeight_",       bobHeight_      );
        ImGuiHelper::OnDrawInputField("bobPeriod_secs_",  bobPeriod_secs_ );
        ImGuiHelper::OnDrawInputField("tiltDegrees_",     tiltDegrees_    );
        ImGuiHelper::OnDrawInputField("tiltPeriod_secs_", tiltPeriod_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::FloatingDrift);
#pragma endregion
