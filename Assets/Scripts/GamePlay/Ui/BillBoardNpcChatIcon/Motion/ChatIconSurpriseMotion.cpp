#include "ChatIconSurpriseMotion.h"

#include <algorithm>
#include <cmath>

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float PI = 3.14159265f;
    }

    void ChatIconSurpriseMotion::OnUpdate()
    {
        const auto rimGlow = rimGlow_.get();
        if (!pop_.Update(*this, popDuration_secs_))
        {
            if (rimGlow)
                rimGlow->SetAlpha(0.0f);
            return;
        }

        const float time = pop_.ShownTime();
        glm::vec3 offset = {};
        offset.y = std::sin(time * floatSpeed_) * floatAmplitude_;

        // NOTE: 枠を走る光の進み具合 (0..1)。負なら光らせない
        float sweepT = -1.0f;
        const float cycleElapsed = std::fmod(time, cycle_secs_);
        if (cycleElapsed < sweepDuration_secs_)
        {
            sweepT = cycleElapsed / sweepDuration_secs_;
        }

        float angle = pop_.BaseAngle();
        const float tiltElapsed = cycleElapsed - (cycle_secs_ - tiltDuration_secs_);
        if (tiltElapsed > 0.0f)
        {
            const float tiltT = tiltElapsed / tiltDuration_secs_;
            angle += std::sin(tiltT * 3.0f * PI) * (1.0f - tiltT) * tiltAngle_;
        }

        pop_.Apply(*this, offset, pop_.PopScale(), angle);

        if (!rimGlow)
            return;

        const int frameCount = rimGlow->GetFrameCount();
        const bool isSweeping = sweepT >= 0.0f && frameCount > 0;
        if (isSweeping)
            rimGlow->SetFrame((std::min)(static_cast<int>(sweepT * static_cast<float>(frameCount)), frameCount - 1));

        rimGlow->SetAngle(angle);
        rimGlow->SetAlpha(isSweeping ? pop_.Alpha() : 0.0f);
    }

    void ChatIconSurpriseMotion::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rimGlow_", rimGlow_);
        ImGuiHelper::OnDrawInputField("popDuration_secs_", popDuration_secs_);
        ImGuiHelper::OnDrawInputField("floatAmplitude_", floatAmplitude_);
        ImGuiHelper::OnDrawInputField("floatSpeed_", floatSpeed_);
        ImGuiHelper::OnDrawInputField("cycle_secs_", cycle_secs_);
        ImGuiHelper::OnDrawInputField("sweepDuration_secs_", sweepDuration_secs_);
        ImGuiHelper::OnDrawInputField("tiltDuration_secs_", tiltDuration_secs_);
        ImGuiHelper::OnDrawInputField("tiltAngle_", tiltAngle_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ChatIconSurpriseMotion);
#pragma endregion
