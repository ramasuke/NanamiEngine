#pragma once
#include <algorithm>
#include <cmath>
#include <memory>

#include "Libs/glm/vec3.hpp"
#include "Libs/glm/gtc/quaternion.hpp"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::CineMachine
{
    class CineMachineVirtualCamera;
}

/** @brief FloatingStone / ReturningIsland / ScatterFloatingStones の演出で共有する部品 */
namespace GamePlay::Prop::StoryMovie
{
    inline float EaseOutCubic (const float t) { return 1.0f - std::pow(1.0f - t, 3.0f); }
    inline float EaseInCubic  (const float t) { return t * t * t; }
    inline float EaseInOutSine(const float t) { return 0.5f - 0.5f * std::cos(t * 3.14159265f); }
    inline float EaseOutBack  (const float t) { const float u = t - 1.0f; return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u; }
    inline float Rate(const float elapsed_secs, const float during_secs) { return std::clamp(elapsed_secs / during_secs, 0.0f, 1.0f); }
    inline glm::quat Yaw(const float degrees) { return glm::angleAxis(glm::radians(degrees), glm::vec3(0.0f, 1.0f, 0.0f)); }

    /** @brief 押しっぱなしで入ってきても即スキップにならないよう、一度離すまで待つ */
    class SkipInput final
    {
    public:
        bool IsSkipped();

    private:
        bool isArmed_ = false;
    };

    /** @brief 子の ParticleSystem をまとめて再生/停止する */
    void SetChildParticlesPlaying(NanamiEngine::Module::GameObject::IGameObject& root, bool isPlaying);

    /** @brief 動かした物のコライダーを、次の Flush で今の位置に作り直させる(Static の Body は Transform に付いてこない) */
    void RebuildColliders(NanamiEngine::Module::GameObject::IGameObject& root);

    void MoveBy(NanamiEngine::Module::GameObject::IGameObject& gameObject, const glm::vec3& offset);

    /** @brief カメラとプレイヤーの操作を演出のあいだだけ借りる */
    class CameraScope final
    {
    public:
        CameraScope(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar,
            std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera,
            std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget,
            const glm::vec3& lookOffset);
        ~CameraScope();

        void Begin() const;
        void End();

    private:
        std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar_;
        std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera_;
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget_;
        glm::vec3 lookOffset_;
        bool isEnded_ = false;
    };
}
