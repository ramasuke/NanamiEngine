#include "EaseFunctor.h"

float LibCore::Tween::EaseFunctor::operator()(const float time) const
{
    return Ease(time);
}

glm::vec3 LibCore::Tween::EaseFunctor::operator()(const float time, const glm::vec3& a, const glm::vec3& b) const
{
    return glm::mix(a, b, Ease(time));
}

glm::quat LibCore::Tween::EaseFunctor::operator()(const float time, const glm::quat& a, const glm::quat& b) const
{
    return glm::slerp(a, b, Ease(time));
}
