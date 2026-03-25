#pragma once
#include <../JoltPhysics/Jolt/Jolt.h>

#include "../JoltPhysics/Jolt/Physics/Body/BodyID.h"
#include "../JoltPhysics/Jolt/Physics/Collision/ContactListener.h"

namespace NanamiEngine::Module::Physics
{
    struct AddContacted final
    {
        AddContacted(
            JPH::BodyID inBody1,
            JPH::BodyID inBody2,
            bool inBody1IsSensor,
            bool inBody2IsSensor,
            void* inUserData1,
            void* inUserData2,
            const JPH::ContactManifold& inManifold)
            : body1(inBody1)
            , body2(inBody2)
            , body1IsSensor(inBody1IsSensor)
            , body2IsSensor(inBody2IsSensor)
            , userData1(inUserData1)
            , userData2(inUserData2)
            , manifold(inManifold)
        {}

        JPH::BodyID body1;
        JPH::BodyID body2;
        bool body1IsSensor;
        bool body2IsSensor;
        void* userData1;
        void* userData2;
        JPH::ContactManifold manifold;
    };
}
