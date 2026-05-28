#include "Engine_Physics_Physics.h"

#include "DxLib.h"
#include "../../Core/Application/ApplicationBase.h"
#include "../../Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../Core/Physics/Physics.h"
#include "ext/quaternion_geometric.hpp"
#include "../JoltPhysics/Jolt/Physics/Collision/CastResult.h"
#include "../JoltPhysics/Jolt/Physics/Collision/CollideShape.h"
#include "../JoltPhysics/Jolt/Physics/Collision/RayCast.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/SphereShape.h"
#include "BroadPhaseLayer/Engine_Physics_NonRaycastLayerFilter.h"
#include "detail/func_trigonometric.inl"
#include "LayerFilter/Engine_Physics_CustomObjectLayerFilter.h"
#include "UserData/Engine_Physics_UserData.h"

inline VECTOR ToDxVec(const glm::vec3& v)
{
    return VGet(v.x, v.y, v.z);
}

JPH::Vec3 MultiplyPointInvCompat(const JPH::RMat44& m, const JPH::Vec3& p)
{
    return m.Multiply3x3Transposed(p - m.GetTranslation());
}

JPH::Vec3 MultiplyVectorCompat(const JPH::RMat44& m, const JPH::Vec3& v)
{
    return m.Multiply3x3(v);
}

glm::vec3 NanamiEngine::Module::Physics::GetLinearVelocity(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    return ToVec3(bodyInterface.GetLinearVelocity(bodyId));
}

void NanamiEngine::Module::Physics::SetLinearVelocity(const JPH::BodyID& bodyId, const glm::vec3& velocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    bodyInterface.SetLinearVelocity(bodyId, ToJPHVec3(velocity));
}

void NanamiEngine::Module::Physics::AddForce(const JPH::BodyID& bodyId, const glm::vec3& velocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();
    const JPH::Vec3 current = bodyInterface.GetLinearVelocity(bodyId);

    bodyInterface.SetLinearVelocity(
        bodyId,
        current + ToJPHVec3(velocity)
    );
}

glm::vec3 NanamiEngine::Module::Physics::GetAngularVelocity(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    // Jolt → rad/s
    const JPH::Vec3 angVelRad = bodyInterface.GetAngularVelocity(bodyId);

    // rad → deg
    return glm::degrees(ToVec3(angVelRad));
}

void NanamiEngine::Module::Physics::SetAngularVelocity(
    const JPH::BodyID& bodyId,
    const glm::vec3& angularVelocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    // deg → rad
    const glm::vec3 rad = glm::radians(angularVelocity);

    bodyInterface.SetAngularVelocity(bodyId, ToJPHVec3(rad));
}

void NanamiEngine::Module::Physics::AddTorque(const JPH::BodyID& bodyId, const glm::vec3& torque)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    bodyInterface.AddTorque(bodyId, ToJPHVec3(torque));
}


JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateBoxShape(const JPH::Vec3& halfSize)
{
    return new JPH::BoxShape(halfSize);
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateCapsuleShape(float halfHeight, float radius)
{
    return new JPH::CapsuleShape(halfHeight, radius);
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateSphereShape(const float radius)
{
    return new JPH::SphereShape(radius);
}

NanamiEngine::Module::Physics::RaycastHit NanamiEngine::Module::Physics::Raycast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    const LayerMask layerMask)
{
    if constexpr (Core::Application::Configuration::APPLICATION_MODE ==
        Core::Application::Configuration::ApplicationMode::Editor)
    {
        DebugDrawRaycast(origin, direction, maxDistance);
    }
    
    const JPH::Vec3 originPos   = ToJPHVec3(origin);
    const JPH::Vec3 jphDirection = ToJPHVec3(glm::normalize(direction));

    JPH::RayCast raycast(originPos, jphDirection * maxDistance);
    JPH::RRayCast rRaycast(raycast);

    JPH::RayCastResult result;
    const auto& physics = Core::Application::ApplicationBase::Physics().GetPhysicsSystem();
    const auto& query   = physics.GetNarrowPhaseQuery();

    if (const CustomObjectLayerFilter layerFilter(layerMask); !query.CastRay(
            rRaycast,
            result,
            JPH::BroadPhaseLayerFilter(),
            layerFilter,
            NonRaycastLayerFilter()))
    {
        return RaycastHit(false, {}, {}, std::shared_ptr<GameObject::IGameObject>());
    }

    // 衝突点
    float dist = maxDistance * result.mFraction;
    JPH::Vec3 hitPosJ = originPos + jphDirection * dist;
    glm::vec3 hitPos  = ToVec3(hitPosJ);

    glm::vec3 hitNormal;
    {
        JPH::BodyLockRead lock(physics.GetBodyLockInterface(), result.mBodyID);
        const JPH::Body& body = lock.GetBody();
        const JPH::Shape* shape = body.GetShape();
        const auto transform = body.GetCenterOfMassTransform();

        // MultiplyPointInv が存在しなければ代替使用
        JPH::Vec3 localPos =
#ifdef JPH_USE_MULTIPLY_POINT_INV
            transform.MultiplyPointInv(hitPosJ);
#else
                MultiplyPointInvCompat(transform, hitPosJ);
#endif

        JPH::Vec3 localNormal = shape->GetSurfaceNormal(result.mSubShapeID2, localPos);

        // MultiplyVector が存在しない場合は Multiply3x3
        JPH::Vec3 worldNormal =
#ifdef JPH_USE_MULTIPLY_VECTOR
            transform.MultiplyVector(localNormal).Normalized();
#else
                MultiplyVectorCompat(transform, localNormal).Normalized();
#endif

        hitNormal = ToVec3(worldNormal);
    }
    
    JPH::BodyID bodyID = result.mBodyID;

    auto& physicsSystem = Core::Application::ApplicationBase::Physics().GetPhysicsSystem();
    const JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), bodyID);
    if (lock.Succeeded())
    {
        const JPH::Body& body = lock.GetBody();
        const auto userData = ToUserData(body.GetUserData());
        
        return RaycastHit(true, hitPos, hitNormal, userData->Entity());
    }
    throw std::runtime_error("Raycast hit failed!");
}

void NanamiEngine::Module::Physics::DebugDrawRaycast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const float maxDistance)
{
    const glm::vec3 end = origin + glm::normalize(direction) * maxDistance;

    DrawLine3D(
        ToDxVec(origin),
        ToDxVec(end),
        GetColor(0, 255, 0)
    );
}
