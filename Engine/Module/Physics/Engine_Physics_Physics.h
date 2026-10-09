#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include "vec3.hpp"
#include "Layer/Engine_Physics_PhysicsLayer.h"
#include "RaycastHit/Engine_Physics_RaycastHit.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Physics
{
    // NOTE: 当たった GameObject から親の RigidBody をさかのぼり、ダメージを受ける持ち主を返す
    [[nodiscard]] NANAMI_API std::shared_ptr<GameObject::IGameObject> FindBodyOwner(const std::shared_ptr<GameObject::IGameObject>& gameObject);

    NANAMI_API RaycastHit Raycast          (const glm::vec3  & origin, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    // NOTE: Distance() は中心が止まる位置までの距離。開始時点で重なっていれば 0
    NANAMI_API RaycastHit SphereCast       (const glm::vec3  & origin, float radius, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    // NOTE: Distance() は中心が止まる位置までの距離。開始時点で重なっていれば 0
    NANAMI_API RaycastHit BoxCast          (const glm::vec3  & origin, const glm::vec3& halfExtents, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    NANAMI_API void DebugDrawRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);
}
