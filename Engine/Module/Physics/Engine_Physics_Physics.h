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

    RaycastHit Raycast          (const glm::vec3  & origin, const glm::vec3& direction, float maxDistance, Layer layer);
    glm::vec3  GetLinearVelocity(const JPH::BodyID& bodyId                           );
    void SetLinearVelocity      (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    // NOTE: deg/s
    glm::vec3 GetAngularVelocity(const JPH::BodyID& bodyId);
    // NOTE: deg/s
    void SetAngularVelocity(const JPH::BodyID& bodyId, const glm::vec3& angularVelocity); 
    void AddForce               (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    void AddTorque(const JPH::BodyID& bodyId, const glm::vec3& torque);

    JPH::RefConst<JPH::Shape> CreateBoxShape    (const JPH::Vec3& halfSize     );
    JPH::RefConst<JPH::Shape> CreateSphereShape (float radius                  );
    JPH::RefConst<JPH::Shape> CreateCapsuleShape(float halfHeight, float radius);
}
