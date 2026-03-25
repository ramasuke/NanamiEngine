#pragma once
#include <cmath>

#include "../Type/EaseType.h"
#include "../glm/fwd.hpp"
#include "../glm/detail/type_quat.hpp"

namespace LibCore::Tween
{
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
