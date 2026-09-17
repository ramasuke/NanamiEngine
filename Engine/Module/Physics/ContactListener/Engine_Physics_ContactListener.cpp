#include "Engine_Physics_ContactListener.h"

#include <cmath>

#include "../../../Core/Application/Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../Component/ComponentBase.h"
#include "../UserData/Engine_Physics_UserData.h"

#include "Jolt/Physics/PhysicsSystem.h"

namespace NanamiEngine::Module::Physics
{
    namespace
    {
        constexpr float CONTACT_LISTENER_DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
    }

    EngineContactListener::EngineContactListener(
        const JPH::PhysicsSystem& physicsSystem)
    : collisionExitGroup_(physicsSystem)
    , sensorExitGroup_(physicsSystem)
    , physicsSystem_(physicsSystem)
    {
        sensorEnterGroup_   .Reserve(1024);
        sensorExitGroup_    .Reserve(1024);
        collisionEnterGroup_.Reserve(1024);
        collisionExitGroup_ .Reserve(1024);
    }

    EngineContactListener::~EngineContactListener() = default;

    void EngineContactListener::OnUpdate()
    {
        sensorEnterGroup_   .Dispatch();
        sensorExitGroup_    .Dispatch();
        collisionEnterGroup_.Dispatch();
        collisionExitGroup_ .Dispatch();
    }

    void EngineContactListener::RefreshTuning()
    {
        using Core::Application::Configuration::PhysicsConfiguration;

        staticFriction_      = PhysicsConfiguration::GetStaticFriction();
        staticFrictionSpeed_ = PhysicsConfiguration::GetStaticFrictionSpeed();
        cosMaxSlope_         = std::cos(PhysicsConfiguration::GetStaticFrictionMaxSlopeDeg() * CONTACT_LISTENER_DEG_TO_RAD);
    }

    void EngineContactListener::OnContactAdded(
    const JPH::Body& body1,
    const JPH::Body& body2,
    const JPH::ContactManifold& manifold,
    JPH::ContactSettings& settings)
    {
        ApplyStaticFriction(body1, body2, manifold, settings);

        const bool isSensor1 = body1.IsSensor();
        const bool isSensor2 = body2.IsSensor();

        if (isSensor1 && isSensor2)
            return;

        //Sensor同士ではない、Sensor + Rigidで発火
        if (isSensor1 != isSensor2)
        {
            const JPH::Body& sensor = isSensor1 ? body1 : body2;
            const JPH::Body& other  = isSensor1 ? body2 : body1;

            const auto sensorPtr = ToUserData(sensor.GetUserData());
            const auto otherPtr  = ToUserData(other .GetUserData());

            assert(sensorPtr);
            assert(otherPtr);

            sensorEnterGroup_.Add({
                .key_ = { sensor.GetID(), other.GetID() },
                .maniFold_ = {
                    {
                        manifold.mWorldSpaceNormal.GetX(),
                        manifold.mWorldSpaceNormal.GetY(),
                        manifold.mWorldSpaceNormal.GetZ()
                    },
                    manifold.mPenetrationDepth
                },
                .sensorUserData_ = sensorPtr,
                .otherUserData_ = otherPtr
            });

            return;
        }
        
        const auto body1Ptr = ToUserData(body1.GetUserData());
        const auto body2Ptr = ToUserData(body2.GetUserData());

        assert(body1Ptr);
        assert(body2Ptr);

        collisionEnterGroup_.Add({
            { body1.GetID(), body2.GetID() },
            {
                {
                    manifold.mWorldSpaceNormal.GetX(),
                    manifold.mWorldSpaceNormal.GetY(),
                    manifold.mWorldSpaceNormal.GetZ()
                },
                manifold.mPenetrationDepth
            },
            body1Ptr,
            body2Ptr
        });
    }

    void EngineContactListener::OnContactPersisted(
        const JPH::Body& body1,
        const JPH::Body& body2,
        const JPH::ContactManifold& manifold,
        JPH::ContactSettings& settings)
    {
        ApplyStaticFriction(body1, body2, manifold, settings);
    }

    void EngineContactListener::ApplyStaticFriction(
        const JPH::Body& body1,
        const JPH::Body& body2,
        const JPH::ContactManifold& manifold,
        JPH::ContactSettings& settings) const
    {
        if (staticFriction_ <= settings.mCombinedFriction)
            return;

        if (body1.IsSensor() || body2.IsSensor())
            return;

        // 壁に押し付けた時に張り付かないよう、床と斜面の接触だけを対象にする
        //NOTE: mWorldSpaceNormal は body2 を押し出す向きで、どちらが地面側かは決まっていないので絶対値で見る
        const JPH::Vec3 normal = manifold.mWorldSpaceNormal;
        if (std::abs(normal.GetY()) < cosMaxSlope_)
            return;

        // 面に沿って動いている間は動摩擦のまま。止まりかけた時だけ静止摩擦へ切り替える
        const JPH::Vec3 relative = body2.GetLinearVelocity() - body1.GetLinearVelocity();
        const JPH::Vec3 tangent  = relative - normal * relative.Dot(normal);
        if (tangent.Length() >= staticFrictionSpeed_)
            return;

        settings.mCombinedFriction = staticFriction_;
    }

    void EngineContactListener::OnContactRemoved(
        const JPH::SubShapeIDPair& pair)
    {
        const PendingExit pendingExit = {
            {
                .a_ = pair.GetBody1ID(),
                .b_ = pair.GetBody2ID()
            }
        };
        
        sensorExitGroup_   .Add(pendingExit);
        collisionExitGroup_.Add(pendingExit);
    }

    void EngineContactListener::UnSubscribeEngineCollider(
        const JPH::BodyID& colliderId)
    {
        collisionEnterGroup_.RemoveByCollider(colliderId);
        collisionExitGroup_ .RemoveByCollider(colliderId);
        sensorEnterGroup_   .RemoveByCollider(colliderId);
        sensorExitGroup_    .RemoveByCollider(colliderId);
    }
}
