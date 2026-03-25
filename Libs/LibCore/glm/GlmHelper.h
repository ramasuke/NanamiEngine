#pragma once
#include "DxLib.h"
#include "fwd.hpp"

namespace LibCore::Glm
{
    glm::mat4 FromDxLibMatrix(const MATRIX& dxMatrix);
}
