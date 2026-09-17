#include "GlmHelper.h"
#include "../glm/glm.hpp"
#include "../glm/mat4x4.hpp"

namespace LibCore
{
    glm::mat4 Glm::FromDxLibMatrix(const MATRIX& dxMatrix)
    {
        // DxLib は行ベクトル規約(平行移動は 4 行目)、glm は列ベクトル規約(平行移動は 4 列目)なので、
        // 格納位置をそのまま写すと転置が打ち消し合って同じ変換になる
        glm::mat4 glmMat(1.0f);
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                glmMat[i][j] = dxMatrix.m[i][j];
            }
        }

        return glmMat;
    }
}