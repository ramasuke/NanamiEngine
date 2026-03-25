#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

// BroadPhaseLayerInterface の簡易実装
class SimpleBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    SimpleBroadPhaseLayerInterface()
    {
        mObjectToBroadPhase[0] = JPH::BroadPhaseLayer(0);
    }

    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override
    {
        return 1;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return mObjectToBroadPhase[inLayer];
    }

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[1];
};
