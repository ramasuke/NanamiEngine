#pragma once
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
    // 当たった GameObject から isPartOfParent_ の RigidBody をさかのぼり、ダメージ等を受ける持ち主の GameObject を返す
    [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindBodyOwner(const std::shared_ptr<GameObject::IGameObject>& gameObject);

    RaycastHit Raycast          (const glm::vec3  & origin, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    // 半径radiusの球をdirectionへmaxDistanceだけ移動させ、最初に当たったコライダーを返す。
    // Distance()は球の中心が止まる位置までの距離。開始時点で既に重なっている場合はDistance()==0。
    RaycastHit SphereCast       (const glm::vec3  & origin, float radius, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    void DebugDrawRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);
}
