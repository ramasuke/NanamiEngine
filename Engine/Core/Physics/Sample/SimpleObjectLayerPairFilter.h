#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "../../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

class NANAMI_API SimpleObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override
   {
       return NanamiEngine::Module::Physics::PhysicsLayers::LayersCollide(
           NanamiEngine::Module::Physics::PhysicsLayers::ToLayer(layer1),
           NanamiEngine::Module::Physics::PhysicsLayers::ToLayer(layer2));
   }
};
