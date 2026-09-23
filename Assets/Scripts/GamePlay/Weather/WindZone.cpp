#include "WindZone.h"
#include <cmath>
#include <DxLib.h>
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Weather
{
    namespace
    {
        // WindZone 未配置時の既定値。GrassField が持っていた値をそのまま引き継いでいる
        constexpr float DEFAULT_DIRECTION_DEG = 30.0f;
        constexpr float DEFAULT_STRENGTH01    = 1.0f;
        constexpr float DEFAULT_SPEED         = 1.6f;
        constexpr float DEFAULT_FREQUENCY     = 0.0015f;

        glm::vec2 ToDirection(const float degree)
        {
            const float radian = degree * DX_PI_F / 180.0f;
            return {std::cos(radian), std::sin(radian)};
        }
    }

    WindZone* WindZone::instance_ = nullptr;

    glm::vec2 WindZone::GetDirection()
    {
        return ToDirection(instance_ ? instance_->windDirectionDeg_ : DEFAULT_DIRECTION_DEG);
    }

    float WindZone::GetStrength01()
    {
        return instance_ ? instance_->strength01_ : DEFAULT_STRENGTH01;
    }

    float WindZone::GetSpeed()
    {
        return instance_ ? instance_->speed_ : DEFAULT_SPEED;
    }

    float WindZone::GetFrequency()
    {
        return instance_ ? instance_->frequency_ : DEFAULT_FREQUENCY;
    }

    void WindZone::InitRenderer()
    {
        instance_ = this;
    }

    void WindZone::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }

    void WindZone::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("windDirectionDeg_", windDirectionDeg_);
        ImGuiHelper::OnDrawInputField("strength01_",       strength01_);
        ImGuiHelper::OnDrawInputField("speed_",            speed_);
        ImGuiHelper::OnDrawInputField("frequency_",        frequency_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Weather::WindZone);
#pragma endregion
