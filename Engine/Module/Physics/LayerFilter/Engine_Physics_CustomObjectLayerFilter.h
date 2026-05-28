#include "../Layer/Engine_Physics_PhysicsLayer.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"

namespace NanamiEngine::Module::Physics
{
    class CustomObjectLayerFilter final : public JPH::ObjectLayerFilter
    {
    public:
        explicit CustomObjectLayerFilter(const LayerMask mask)
            : mask_(mask)
        {}

        [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer objectLayer) const override
        {
            return (mask_ & (1u << objectLayer)) != 0;
        }

    private:
        LayerMask mask_;
    };
}
