#pragma once
#include <algorithm>
#include <cmath>

#include "Libs/glm/vec3.hpp"
#include "Libs/glm/gtc/quaternion.hpp"
#include "Prop_StoryMovieCameraScope.h"
#include "Prop_StoryMovieSkipInput.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

// NOTE: 物語演出のコルーチンで共有する部品
namespace GamePlay::Prop::StoryMovie
{
    inline float EaseOutCubic (const float t) { return 1.0f - std::pow(1.0f - t, 3.0f); }
    inline float EaseInCubic  (const float t) { return t * t * t; }
    inline float EaseInOutSine(const float t) { return 0.5f - 0.5f * std::cos(t * 3.14159265f); }
    inline float EaseOutBack  (const float t) { const float u = t - 1.0f; return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u; }
    inline float Rate(const float elapsed_secs, const float during_secs) { return std::clamp(elapsed_secs / during_secs, 0.0f, 1.0f); }
    inline glm::quat Yaw(const float degrees) { return glm::angleAxis(glm::radians(degrees), glm::vec3(0.0f, 1.0f, 0.0f)); }

    // NOTE: 子の ParticleSystem をまとめて再生/停止する
    void SetChildParticlesPlaying(NanamiEngine::Module::GameObject::IGameObject& root, bool isPlaying);

    // NOTE: 動かした物のコライダーを、次の Flush で今の位置に作り直させる(Static の Body は Transform に付いてこない)
    void RebuildColliders(NanamiEngine::Module::GameObject::IGameObject& root);

    void MoveBy(NanamiEngine::Module::GameObject::IGameObject& gameObject, const glm::vec3& offset);
}
