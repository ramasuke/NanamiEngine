#include "GlmHelper.h"
#include "../glm/glm.hpp"
#include "../glm/mat4x4.hpp"

namespace LibCore
{
    glm::mat4 Glm::FromDxLibMatrix(const MATRIX& dxMatrix)
    {
        glm::mat4 glmMat;
        // DXLib(row-major) → GLM(column-major)
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                glmMat[col][row] = dxMatrix.m[row][col];
            }
        }

        return glmMat;
    }
}