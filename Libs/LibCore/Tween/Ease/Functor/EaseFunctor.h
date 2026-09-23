#pragma once
#include <cmath>
#include <numbers>

#include "../Type/EaseType.h"
#include "../glm/fwd.hpp"
#include "../glm/detail/type_quat.hpp"
#include "../../../../../Engine/Module/Color/Color32.h"

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
                return 1.0f + (overshoot_ + 1.0f) * u * u * u + overshoot_ * u * u;
            }
            case EaseType::InBack:
                return (overshoot_ + 1.0f) * t * t * t - overshoot_ * t * t;
            case EaseType::InOutSine:
                return 0.5f - 0.5f * std::cos(t * std::numbers::pi_v<float>);
            case EaseType::OutCubic:
            {
                const float u = 1.0f - t;
                return 1.0f - u * u * u;
            }
            case EaseType::InCubic:
                return t * t * t;
            case EaseType::InOutCubic:
                return (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f);
            case EaseType::SmoothStep:
                return t * t * (3.0f - 2.0f * t);
            case EaseType::Linear:
            default:
                return t;
            }
        }

        float     operator()(float time) const;
        float     operator()(float time, float a, float b) const;
        glm::vec3 operator()(float time, const glm::vec3& a, const glm::vec3& b) const;
        glm::quat operator()(float time, const glm::quat& a, const glm::quat& b) const;
        // Color32 は算術演算を持たないので、.via(Tween::Ease(...)) を付けないと tweeny の既定イージングで start のまま動かない
        NanamiEngine::Color32 operator()(float time, const NanamiEngine::Color32& a, const NanamiEngine::Color32& b) const;
        EaseType easing_;
        // Back系だけが使う
        float overshoot_ = EASE_BACK_OVERSHOOT;
    };
}
