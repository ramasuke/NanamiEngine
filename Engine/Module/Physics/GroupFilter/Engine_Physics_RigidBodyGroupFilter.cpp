#include "Engine_Physics_RigidBodyGroupFilter.h"

#include "Jolt/Physics/Collision/CollisionGroup.h"

namespace NanamiEngine::Module::Physics
{
    bool RigidBodyGroupFilter::CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const
    {
        if (group1.GetGroupID() == JPH::CollisionGroup::cInvalidGroup || group2.GetGroupID() == JPH::CollisionGroup::cInvalidGroup)
            return true;

        return group1.GetGroupID() != group2.GetGroupID();
    }
}
