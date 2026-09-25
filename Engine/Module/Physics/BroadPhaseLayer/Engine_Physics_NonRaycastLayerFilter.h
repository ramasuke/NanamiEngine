#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyFilter.h"

namespace NanamiEngine::Module::Physics
{
    class NANAMI_API NonRaycastLayerFilter final : public JPH::BodyFilter
    {
    public:
        bool ShouldCollide(const JPH::BodyID& inBodyID) const override;
    };
}
