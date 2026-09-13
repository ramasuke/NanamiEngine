#pragma once
#include <array>

#include "vec2.hpp"

namespace GameCore::PathFinding
{
    constexpr std::array<glm::ivec2, 8> EIGHT_DIRECTIONS
    {{
        { 1,  0}, {-1,  0}, { 0,  1}, { 0, -1},
        { 1,  1}, { 1, -1}, {-1,  1}, {-1, -1},
    }};
}
