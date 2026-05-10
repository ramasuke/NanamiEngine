#pragma once
#include <vector>
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Collision/ContactListener.h>

#include "CollisionEnterGroup/Engine_Physics_CollisionEnterGroup.h"
#include "CollisionExitGroup/Engine_Physics_CollisionExitGroup.h"
#include "ContactedData/Engine_Physics_ContactedData.h"
#include "SensorEnterGroup/Engine_Physics_SensorEnterGroup.h"
#include "SensorExitGroup/Engine_Physics_SensorExitGroup.h"

namespace JPH
{
    class PhysicsSystem;
}

namespace NanamiEngine::Module::Physics
{
    class EngineContactListener final : public JPH::ContactListener
    {
    public:
        explicit EngineContactListener(const JPH::PhysicsSystem& physicsSystem);
        ~EngineContactListener() override;
        //NOTE: メインスレッド呼び出しを推奨
        void OnUpdate();
        
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId);

    private:
        void OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings&) override;
        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override;

        CollisionEnterGroup collisionEnterGroup_;
        CollisionExitGroup  collisionExitGroup_;
        SensorEnterGroup sensorEnterGroup_;
        SensorExitGroup  sensorExitGroup_;
        const JPH::PhysicsSystem& physicsSystem_;
    };
}