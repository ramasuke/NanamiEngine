#include "Engine_Physics_ContactListener.h"

#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../Component/ComponentBase.h"
#include "../UserData/Engine_Physics_UserData.h"

#include "Jolt/Physics/PhysicsSystem.h"

namespace NanamiEngine::Module::Physics
{
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

    void EngineContactListener::OnContactAdded(
    const JPH::Body& body1,
    const JPH::Body& body2,
    const JPH::ContactManifold& manifold,
    JPH::ContactSettings&)
    {
        const bool isSensor1 = body1.IsSensor();
        const bool isSensor2 = body2.IsSensor();

        if (isSensor1 && isSensor2)
            return;

        //Sensor同士ではない、Sensor + Rigidで発火
        if (isSensor1 != isSensor2)
        {
            const JPH::Body& sensor = isSensor1 ? body1 : body2;
            const JPH::Body& other  = isSensor1 ? body2 : body1;

            const auto sensorPtr = reinterpret_cast<UserData*>(sensor.GetUserData());
            const auto otherPtr  = reinterpret_cast<UserData*>(other .GetUserData());

            assert(sensorPtr);
            assert(otherPtr);

            sensorEnterGroup_.Add({
                { sensor.GetID(), other.GetID() },
                {
                    {
                        manifold.mWorldSpaceNormal.GetX(),
                        manifold.mWorldSpaceNormal.GetY(),
                        manifold.mWorldSpaceNormal.GetZ()
                    },
                    manifold.mPenetrationDepth
                },
                sensorPtr,
                otherPtr
            });

            return;
        }
        
        const auto body1Ptr = reinterpret_cast<UserData*>(body1.GetUserData());
        const auto body2Ptr = reinterpret_cast<UserData*>(body2.GetUserData());

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
