#pragma once
#include "DxLib.h"
#include "fwd.hpp"

//NOTE: glm と DxLib の型の変換。DxLib を呼ぶ .cpp からだけ include する(公開ヘッダには出さない)
namespace LibCore::Dxlib
{
    VECTOR    ToDxVector  (const glm::vec3& vector);
    glm::vec3 FromDxVector(const VECTOR& vector);
    MATRIX    ToDxMatrix  (const glm::mat4& matrix);
    glm::mat4 FromDxMatrix(const MATRIX& matrix);
}
