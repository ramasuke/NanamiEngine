#pragma once
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Collision/ContactListener.h>

#include "ContactedData/ContactedData.h"

namespace NanamiEngine::Module::Physics
{
    class EngineContactListener final : public JPH::ContactListener
    {
    public:
        EngineContactListener();
        void OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings&) override;

        void OnUpdate();
        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override;
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId);

    private:
        //TODO: pendingする必要がないため削除必須
        std::vector<PendingEnter> pendingEnter_;
        std::vector<ContactKey  > pendingExit_;

        std::unordered_map<ContactKey, CachedContact, ContactKeyHash> activeSensorContacts_;

        void TryTriggerEnter();
        void TryTriggerExit();
    };
}
