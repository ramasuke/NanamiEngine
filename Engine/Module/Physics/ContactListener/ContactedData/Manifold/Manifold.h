#pragma once
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Math/Vec3.h"

namespace NanamiEngine::Module::Physics
{
    struct Manifold
    {
        JPH::Vec3 normal_;
        float penetration_;
    };
}
