#pragma once
#include <utility>
#include "vec3.hpp"
#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"

// エンジン内部専用。ゲームコードは RigidBody / ICollider の公開 API を使うこと
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

    glm::vec3 GetCenterOfMassPosition(const JPH::BodyID& bodyId);
    // ボディのワールド空間AABB(first=min, second=max)。無効なBodyIDは呼び出し側で弾くこと
    std::pair<glm::vec3, glm::vec3> GetWorldSpaceBounds(const JPH::BodyID& bodyId);
    glm::vec3  GetLinearVelocity(const JPH::BodyID& bodyId                           );
    void SetLinearVelocity      (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    // deg/s
    glm::vec3 GetAngularVelocity(const JPH::BodyID& bodyId);
    // deg/s
    void SetAngularVelocity(const JPH::BodyID& bodyId, const glm::vec3& angularVelocity);
    void AddLinearVelocity      (const JPH::BodyID& bodyId, const glm::vec3& velocity);
    void AddTorque(const JPH::BodyID& bodyId, const glm::vec3& torque);

    JPH::RefConst<JPH::Shape> CreateBoxShape      (const JPH::Vec3& halfSize     );
    JPH::RefConst<JPH::Shape> CreateSphereShape  (float radius                  );
    JPH::RefConst<JPH::Shape> CreateCapsuleShape (float halfHeight, float radius);
    JPH::RefConst<JPH::Shape> CreateCylinderShape(float halfHeight, float radius);
}
