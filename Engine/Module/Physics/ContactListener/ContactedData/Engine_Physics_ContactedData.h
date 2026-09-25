#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <../JoltPhysics/Jolt/Jolt.h>
#include <../JoltPhysics/Jolt/Physics/Body/BodyID.h>

#include "Manifold/Engine_Physics_Manifold.h"

namespace NanamiEngine::Module::Physics
{
    struct UserData;
}

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace NanamiEngine::Module::Physics
{
    struct NANAMI_API ContactKey final
    {
        JPH::BodyID a_;
        JPH::BodyID b_;

        bool operator==(const ContactKey& rhs) const;
    };

    struct NANAMI_API ContactKeyHash final
    {
        size_t operator()(const ContactKey& k) const;
    };
    
    struct NANAMI_API CachedContact final
    {
        void* sensorUserData_;
        void* otherUserData_ ;
    };

    struct NANAMI_API PendingExit final 
    {
        ContactKey key_;
    };
    
    struct NANAMI_API PendingEnter final
    {
        ContactKey key_;
        Manifold maniFold_;
        UserData* sensorUserData_;
        UserData* otherUserData_;
    };
}
