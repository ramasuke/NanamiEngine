#pragma once
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    enum class ColliderShapeKind : uint32_t
    {
        Box = 0,
        Sphere,
        Capsule,
        Cylinder,
        StaticMesh,
        Count
    };

    static constexpr const char* COLLIDER_SHAPE_KIND_NAMES[] = {
        "Box",
        "Sphere",
        "Capsule",
        "Cylinder",
        "StaticMesh"
    };

    static_assert(
        static_cast<int>(ColliderShapeKind::Count) == sizeof(COLLIDER_SHAPE_KIND_NAMES) / sizeof(const char*),
        "ColliderShapeKindNames count must match ColliderShapeKind enum count!"
    );
}
