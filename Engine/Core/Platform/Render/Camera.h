#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec3.hpp"

// 描画中のカメラへの問い合わせ (DxLib の GetCameraPosition / ConvWorldPosToScreenPos などの DxLib を出さない入口)
namespace NanamiEngine::Platform::Render::Camera
{
    [[nodiscard]] NANAMI_API glm::vec3 Position();
    /** @brief 垂直視野角 (ラジアン) */
    [[nodiscard]] NANAMI_API float     Fov();
    [[nodiscard]] NANAMI_API glm::vec3 UpVector();
    /** @brief ワールド座標 -> 画面座標。z は 0..1 の深度で、範囲外なら視界の外 (背後など) */
    [[nodiscard]] NANAMI_API glm::vec3 WorldToScreen(const glm::vec3& worldPosition);
    /** @brief 画面座標 (z = 0 でニア面、1 でファー面) -> ワールド座標 */
    [[nodiscard]] NANAMI_API glm::vec3 ScreenToWorld(const glm::vec3& screenPosition);
    /** @brief AABB が視錐台の完全に外か */
    [[nodiscard]] NANAMI_API bool      IsBoxOutsideView(const glm::vec3& boxMin, const glm::vec3& boxMax);
}
