#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <../JoltPhysics/Jolt/Jolt.h>

#include "../JoltPhysics/Jolt/Physics/Body/BodyID.h"

namespace NanamiEngine::Module::Physics
{
    struct NANAMI_API RemoveContacted final
    {
        RemoveContacted(
            const JPH::BodyID inBody1,
            const JPH::BodyID inBody2)
            : body1_(inBody1)
            , body2_(inBody2)
        {}

        JPH::BodyID body1_;
        JPH::BodyID body2_;
    };
}
