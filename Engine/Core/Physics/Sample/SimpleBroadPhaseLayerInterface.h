#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

// BroadPhaseLayerInterface の簡易実装
class NANAMI_API SimpleBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override
    {
        return 1;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return JPH::BroadPhaseLayer(0);
    }
};
