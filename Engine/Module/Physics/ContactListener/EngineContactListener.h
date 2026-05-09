#pragma once
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Collision/ContactListener.h>

#include "ContactedData/ContactedData.h"

namespace JPH
{
    class PhysicsSystem;
}

namespace NanamiEngine::Module::Physics
{
    class EngineContactListener final : public JPH::ContactListener
    {
    public:
        explicit EngineContactListener(
            const JPH::PhysicsSystem& physicsSystem);
        ~EngineContactListener();
        void OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings&) override;

        void OnUpdate();
        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override;
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId);

    private:
        std::vector<PendingEnter> pendingEnter_;
        std::vector<PendingExit> pendingExit_;

        void TryTriggerEnter();
        void TryTriggerExit();
        
        const JPH::PhysicsSystem& physicsSystem_;  
    };
}
