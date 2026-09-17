#pragma once
#include <optional>
#include <utility>
#include "vec3.hpp"

namespace NanamiEngine::Module::Physics
{
    class ICollider
    {
    public:
        virtual ~ICollider() = default;
        // この Collider の形状のワールド空間AABB(first=min, second=max)。Body が無ければ nullopt
        [[nodiscard]] virtual std::optional<std::pair<glm::vec3, glm::vec3>> WorldBounds() const = 0;
        [[nodiscard]] virtual std::optional<glm::vec3> CenterOfMassPosition() const = 0;
    };
}
