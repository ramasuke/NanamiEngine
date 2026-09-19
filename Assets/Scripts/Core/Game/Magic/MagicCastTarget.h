#pragma once
#include "vec3.hpp"
#include "gtc/quaternion.hpp"

namespace GameCore::Magic
{
    // 撃った人の画面で決めた狙い。RPC でそのまま他の画面へ送り、全員が同じ場所へ撃つ
    struct MagicCastTarget final
    {
        glm::vec3 origin    {0.0f};
        glm::quat rotation  {1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 targetPos {0.0f};
        float     powerRate = 1.0f;
    };
}
