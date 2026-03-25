#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"

class SimpleObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override
   {
       return true;
   }
};
