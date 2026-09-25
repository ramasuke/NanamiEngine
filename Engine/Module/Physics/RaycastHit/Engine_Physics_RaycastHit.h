#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Physics
{
    class NANAMI_API RaycastHit final
    {
    public:
        RaycastHit(
            bool hit,
            const glm::vec3& position,
            const glm::vec3& normal,
            float distance,
            const std::weak_ptr<GameObject::IGameObject>& hitObject);

        [[nodiscard]] bool Hit() const;
        [[nodiscard]] const glm::vec3& Position() const { return position_; }
        [[nodiscard]] const glm::vec3& Normal() const { return normal_; }
        // 始点から衝突までの移動距離。SphereCast/BoxCastでは接触点ではなく「形状の中心が止まる位置」までの距離
        [[nodiscard]] float Distance() const { return distance_; }
        [[nodiscard]] GameObject::IGameObject& HitObject() const { return *hitObject_.lock(); }

    private:
        bool hit_ = false;
        glm::vec3 position_{0.0f};
        glm::vec3 normal_{0.0f};
        float distance_ = 0.0f;
        std::weak_ptr<GameObject::IGameObject> hitObject_;
    };
}
