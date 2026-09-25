#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/GroupFilter.h"

namespace NanamiEngine::Module::Physics
{
    // 同じ RigidBody に属する Body(本体と、その下の Sensor)同士を当たらないようにする
    class NANAMI_API RigidBodyGroupFilter final : public JPH::GroupFilter
    {
    public:
        [[nodiscard]] bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override;
    };
}
