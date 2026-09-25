#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Math/Vec3.h"

namespace NanamiEngine::Module::Physics
{
    struct NANAMI_API Manifold final
    {
        JPH::Vec3 normal_;
        float penetration_;
    };
}
