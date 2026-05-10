#pragma once
#include "vec3.hpp"

namespace NanamiEngine::Module::Physics
{
    class RaycastHit final
    {
    public:
        RaycastHit(bool hit, const glm::vec3& position, const glm::vec3& normal);

        [[nodiscard]] bool Hit() const { return hit_; }
        [[nodiscard]] const glm::vec3& Position() const { return position_; }
        [[nodiscard]] const glm::vec3& Normal() const { return normal_; }

    private:
        bool hit_ = false;
        glm::vec3 position_{0.0f};
        glm::vec3 normal_{0.0f};
    };
}
