#include "DxMath.h"
#include "glm.hpp"
#include "BlendMode.h"

namespace LibCore::Dxlib
{
    static_assert(static_cast<int>(BlendMode::NoBlend) == DX_BLENDMODE_NOBLEND);
    static_assert(static_cast<int>(BlendMode::Alpha)   == DX_BLENDMODE_ALPHA);
    static_assert(static_cast<int>(BlendMode::Add)     == DX_BLENDMODE_ADD);
    static_assert(static_cast<int>(BlendMode::Sub)     == DX_BLENDMODE_SUB);
    static_assert(static_cast<int>(BlendMode::Mul)     == DX_BLENDMODE_MUL);

    VECTOR ToDxVector(const glm::vec3& vector)
    {
        return VGet(vector.x, vector.y, vector.z);
    }

    glm::vec3 FromDxVector(const VECTOR& vector)
    {
        return { vector.x, vector.y, vector.z };
    }

    // DxLib は行ベクトル規約(平行移動は 4 行目)、glm は列ベクトル規約(平行移動は 4 列目)なので、
    // 格納位置をそのまま写すと転置が打ち消し合って同じ変換になる
    MATRIX ToDxMatrix(const glm::mat4& matrix)
    {
        MATRIX dxMatrix;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                dxMatrix.m[i][j] = matrix[i][j];
        return dxMatrix;
    }

    glm::mat4 FromDxMatrix(const MATRIX& matrix)
    {
        glm::mat4 glmMatrix(1.0f);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                glmMatrix[i][j] = matrix.m[i][j];
        return glmMatrix;
    }
}
