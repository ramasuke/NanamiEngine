#pragma once
#include "vec3.hpp"
#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Layer/Engine_Physics_PhysicsLayer.h"
#include "RaycastHit/Engine_Physics_RaycastHit.h"

namespace JPH
{
    class Shape;
}

namespace JPH
{
    class RayCastResult;
}

namespace NanamiEngine::Module::Physics
{
    inline JPH::Vec3 ToJPHVec3(const glm::vec3& v)
    {
        return {v.x, v.y, v.z};
    }
    inline glm::vec3 ToVec3(const JPH::Vec3& v)
    {
        return {v.GetX(), v.GetY(), v.GetZ()};
    }

    RaycastHit Raycast          (const glm::vec3  & origin, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    // 半径radiusの球をdirectionへmaxDistanceだけ移動させ、最初に当たったコライダーを返す。
    // Distance()は球の中心が止まる位置までの距離。開始時点で既に重なっている場合はDistance()==0。
    RaycastHit SphereCast       (const glm::vec3  & origin, float radius, const glm::vec3& direction, float maxDistance, LayerMask layerMask);
    // centerから最も近いコライダー表面までの距離を返す。maxDistance以内に何もなければmaxDistance。
    float ClosestDistance       (const glm::vec3  & center, float maxDistance, LayerMask layerMask);
    void DebugDrawRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);
    glm::vec3 GetCenterOfMassPosition(const JPH::BodyID& bodyId);
    glm::vec3  GetLinearVelocity(const JPH::BodyID& bodyId                           );
    void SetLinearVelocity      (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    // deg/s
    glm::vec3 GetAngularVelocity(const JPH::BodyID& bodyId);
    // deg/s
    void SetAngularVelocity(const JPH::BodyID& bodyId, const glm::vec3& angularVelocity); 
    void AddForce               (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    void AddTorque(const JPH::BodyID& bodyId, const glm::vec3& torque);

    JPH::RefConst<JPH::Shape> CreateBoxShape      (const JPH::Vec3& halfSize     );
    JPH::RefConst<JPH::Shape> CreateSphereShape  (float radius                  );
    JPH::RefConst<JPH::Shape> CreateCapsuleShape (float halfHeight, float radius);
    JPH::RefConst<JPH::Shape> CreateCylinderShape(float halfHeight, float radius);
}
