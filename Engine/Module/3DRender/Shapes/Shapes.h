#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <array>

#include "fwd.hpp"
#include "vec3.hpp"
#include "../../Color/Color32.h"

namespace NanamiEngine::Module::Render3D::Shapes
{
    /** @brief 3D 空間の線 (デバッグ描画用) */
    NANAMI_API void DrawLine3D(const glm::vec3& from, const glm::vec3& to, const Color32& color);
    /** @brief 8頂点から立方体を描画する関数 */
    NANAMI_API void DrawCube3DFromVertices(const std::array<glm::vec3, 8>& vertices, const int& edgeColor);
    /** @brief カプセルのワイヤーフレーム描画 */
    NANAMI_API void DrawCapsule3D(const glm::vec3& center,
                       float radius,
                       float halfHeight,
                       const glm::quat& rotation,
                       const int& color);
    /** @brief 円柱のワイヤーフレーム描画 */
    NANAMI_API void DrawCylinder3D(const glm::vec3& center,
                        float radius,
                        float halfHeight,
                        const glm::quat& rotation,
                        const int& color);
    /** @brief 球のワイヤーフレーム描画 */
    NANAMI_API void DrawSphere3D(const glm::vec3& center,
                      float radius,
                      const int& color);
    
    /** @brief Meshワイヤーフレーム描画 */
    NANAMI_API void DrawMeshWireFrame3D(
        const std::vector<glm::vec3>& vertices,
        const std::vector<uint32_t>&  indices,
        const glm::mat4& world,
        const int& color);
}
