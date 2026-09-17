#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "../../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

class SimpleObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override
   {
       return NanamiEngine::Module::Physics::LayersCollide(
           NanamiEngine::Module::Physics::ToLayer(layer1),
           NanamiEngine::Module::Physics::ToLayer(layer2));
   }
};
