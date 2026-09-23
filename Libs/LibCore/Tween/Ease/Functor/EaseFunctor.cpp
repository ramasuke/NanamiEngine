#include "EaseFunctor.h"

float LibCore::Tween::EaseFunctor::operator()(const float time) const
{
    return Ease(time);
}

float LibCore::Tween::EaseFunctor::operator()(const float time, const float a, const float b) const
{
    return a + (b - a) * Ease(time);
}

glm::vec3 LibCore::Tween::EaseFunctor::operator()(const float time, const glm::vec3& a, const glm::vec3& b) const
{
    return glm::mix(a, b, Ease(time));
}

glm::quat LibCore::Tween::EaseFunctor::operator()(const float time, const glm::quat& a, const glm::quat& b) const
{
    return glm::slerp(a, b, Ease(time));
}

NanamiEngine::Color32 LibCore::Tween::EaseFunctor::operator()(const float time, const NanamiEngine::Color32& a, const NanamiEngine::Color32& b) const
{
    // FromVec3 が 0..1 に clamp するので、Back 系の行き過ぎでも 0..255 に収まる
    return NanamiEngine::Color32::FromVec3(glm::mix(a.ToVec3(), b.ToVec3(), Ease(time)));
}
