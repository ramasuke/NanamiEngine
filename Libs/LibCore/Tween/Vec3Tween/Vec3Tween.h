#pragma once
#include "../glm/vec3.hpp"
#include "../tweeny/Tweeny/tweeny.h"

namespace LibCore::Tween
{
    class Vec3Tween final
    {
    public:
        explicit Vec3Tween(tweeny::tween<glm::vec3>& tween);
        
    private:
        tweeny::tween<glm::vec3>& tween_;
    };
}
