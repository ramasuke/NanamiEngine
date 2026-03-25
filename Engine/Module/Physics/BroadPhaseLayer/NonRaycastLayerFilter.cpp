#include "NonRaycastLayerFilter.h"

#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Physics/Physics.h"
#include "Jolt/Physics/Body/BodyLock.h"


namespace NanamiEngine::Module::Physics
{
    bool NonRaycastLayerFilter::ShouldCollide(const JPH::BodyID& inBodyID) const
    {
        const auto& physics = Core::Application::ApplicationBase::Physics().GetPhysicsSystem(); 
        const JPH::BodyLockRead bodyLocked(physics.GetBodyLockInterface(), inBodyID);
        if (bodyLocked.Succeeded())
        {
            const JPH::Body& body = bodyLocked.GetBody();
            return !body.IsSensor();
        }

        return false;
    }
}
