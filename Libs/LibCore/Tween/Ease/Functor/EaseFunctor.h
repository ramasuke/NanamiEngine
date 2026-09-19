#pragma once
#include <cmath>
#include <numbers>

#include "../Type/EaseType.h"
#include "../glm/fwd.hpp"
#include "../glm/detail/type_quat.hpp"

namespace LibCore::Tween
{
    // Back系が一度行き過ぎる量。Robert Pennerの式の定番値で、約10%はみ出す
    constexpr float EASE_BACK_OVERSHOOT = 1.70158f;

    struct EaseFunctor
    {
        [[nodiscard]] float Ease(const float t) const
        {
            switch (easing_)
            {
            case EaseType::OutQuad:
                return 1.0f - (1.0f - t) * (1.0f - t);
            case EaseType::InQuad:
                return t * t;
            case EaseType::InOutQuad:
                return (t < 0.5f) ? (2*t*t) : (1 - std::pow(-2*t + 2, 2) / 2);
            case EaseType::OutBack:
            {
                const float u = t - 1.0f;
                return 1.0f + (EASE_BACK_OVERSHOOT + 1.0f) * u * u * u + EASE_BACK_OVERSHOOT * u * u;
            }
            case EaseType::InBack:
                return (EASE_BACK_OVERSHOOT + 1.0f) * t * t * t - EASE_BACK_OVERSHOOT * t * t;
            case EaseType::InOutSine:
                return 0.5f - 0.5f * std::cos(t * std::numbers::pi_v<float>);
            case EaseType::Linear:
            default:
                return t;
            }
        }

        float     operator()(float time) const;
        glm::vec3 operator()(float time, const glm::vec3& a, const glm::vec3& b) const;
        glm::quat operator()(float time, const glm::quat& a, const glm::quat& b) const;
        EaseType easing_;
    };
}
