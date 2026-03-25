#pragma once
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Body/BodyID.h>

#include "../JoltPhysics/Jolt/Physics/Collision/ContactListener.h"
#include "Manifold/Manifold.h"

namespace NanamiEngine::Module::Physics
{
    struct ContactKey
    {
        JPH::BodyID a_;
        JPH::BodyID b_;

        bool operator==(const ContactKey& rhs) const;
    };

    struct ContactKeyHash
    {
        size_t operator()(const ContactKey& k) const;
    };
    
    struct CachedContact
    {
        void* sensorUserData_;
        void* otherUserData_;
    };

    
    
    struct PendingEnter
    {
        ContactKey key_;
        Manifold maniFold_;
    };
}
