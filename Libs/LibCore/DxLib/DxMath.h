#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "DxLib.h"
#include "fwd.hpp"

//NOTE: glm と DxLib の型の変換。DxLib を呼ぶ .cpp からだけ include する(公開ヘッダには出さない)
namespace LibCore::Dxlib
{
    NANAMI_API VECTOR    ToDxVector  (const glm::vec3& vector);
    NANAMI_API glm::vec3 FromDxVector(const VECTOR& vector);
    NANAMI_API MATRIX    ToDxMatrix  (const glm::mat4& matrix);
    NANAMI_API glm::mat4 FromDxMatrix(const MATRIX& matrix);
}
