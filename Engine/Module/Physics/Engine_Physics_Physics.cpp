#include "Engine_Physics_Physics.h"

#include <algorithm>

#include "DxLib.h"
#include "../../Core/Application/ApplicationBase.h"
#include "../../Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../Core/Physics/Physics.h"
#include "ext/quaternion_geometric.hpp"
#include "../JoltPhysics/Jolt/Physics/Collision/CastResult.h"
#include "../JoltPhysics/Jolt/Physics/Collision/CollideShape.h"
#include "../JoltPhysics/Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "../JoltPhysics/Jolt/Physics/Collision/RayCast.h"
#include "../JoltPhysics/Jolt/Physics/Collision/ShapeCast.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/SphereShape.h"
#include "../GameObject/Interface/IGameObject.h"
#include "../GameObject/Transform/Transform.h"
#include "BroadPhaseLayer/Engine_Physics_NonRaycastLayerFilter.h"
#include "Component/RigidBody/Engine_Physics_RigidBody.h"
#include "JoltUtility/Engine_Physics_JoltUtility.h"
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

std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> NanamiEngine::Module::Physics::FindBodyOwner(
    const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    auto owner = gameObject;
    while (owner)
    {
        const auto rigidBody = owner->Components().Catch<Component::RigidBody>().lock();
        if (!rigidBody || !rigidBody->IsPartOfParent())
            return owner;

        auto parent = owner->Transform().GetParent();
        while (parent && parent->Components().Catch<Component::RigidBody>().expired())
            parent = parent->Transform().GetParent();

        if (!parent)
            return owner;
        owner = parent;
    }
    return owner;
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
        return RaycastHit(false, {}, {}, 0.0f, std::shared_ptr<GameObject::IGameObject>());
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
        
        return RaycastHit(true, hitPos, hitNormal, dist, userData->Entity());
    }
    throw std::runtime_error("Raycast hit failed!");
}

NanamiEngine::Module::Physics::RaycastHit NanamiEngine::Module::Physics::SphereCast(
    const glm::vec3& origin,
    const float radius,
    const glm::vec3& direction,
    const float maxDistance,
    const LayerMask layerMask)
{
    if constexpr (Core::Application::Configuration::APPLICATION_MODE ==
        Core::Application::Configuration::ApplicationMode::Editor)
    {
        DebugDrawRaycast(origin, direction, maxDistance);
    }

    const glm::vec3 normalizedDirection = glm::normalize(direction);

    const JPH::SphereShape sphere(radius);
    // スタック上のShapeを参照カウントで破棄させないためのガード
    sphere.SetEmbedded();

    const JPH::RShapeCast shapeCast(
        &sphere,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(ToJPHVec3(origin)),
        ToJPHVec3(normalizedDirection * maxDistance));

    JPH::ShapeCastSettings settings;
    // 片面メッシュ(地形など)を裏側からすり抜けないよう、裏面にも当てる
    settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
    settings.mReturnDeepestPoint = true;

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    const auto& physics = Core::Application::ApplicationBase::Physics().GetPhysicsSystem();

    const CustomObjectLayerFilter layerFilter(layerMask);
    physics.GetNarrowPhaseQuery().CastShape(
        shapeCast,
        settings,
        JPH::RVec3::sZero(),
        collector,
        JPH::BroadPhaseLayerFilter(),
        layerFilter,
        NonRaycastLayerFilter());

    if (!collector.HadHit())
    {
        return RaycastHit(false, {}, {}, 0.0f, std::shared_ptr<GameObject::IGameObject>());
    }

    const JPH::ShapeCastResult& result = collector.mHit;
    const float     hitDistance = maxDistance * result.mFraction;
    const glm::vec3 hitPos      = ToVec3(result.mContactPointOn2);
    const JPH::Vec3 axis        = result.mPenetrationAxis;
    const glm::vec3 hitNormal   = axis.IsNearZero() ? -normalizedDirection : ToVec3(-axis.Normalized());

    const JPH::BodyLockRead lock(physics.GetBodyLockInterface(), result.mBodyID2);
    if (!lock.Succeeded())
    {
        return RaycastHit(false, {}, {}, 0.0f, std::shared_ptr<GameObject::IGameObject>());
    }

    const auto userData = ToUserData(lock.GetBody().GetUserData());
    return RaycastHit(true, hitPos, hitNormal, hitDistance, userData->Entity());
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
