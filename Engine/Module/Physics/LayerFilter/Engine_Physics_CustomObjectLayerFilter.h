#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"

namespace NanamiEngine::Module::Physics
{
    class CustomObjectLayerFilter final : public JPH::ObjectLayerFilter
    {
    public:
        explicit CustomObjectLayerFilter(const JPH::ObjectLayer layer)
            : layer_(layer)
        {
        }

        [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer objectLayer) const override
        {
            return objectLayer == layer_;
        }

    private:
        JPH::ObjectLayer layer_;
    };
}
