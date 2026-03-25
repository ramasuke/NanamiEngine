#pragma once
#include <array>

#include "fwd.hpp"
#include "vec3.hpp"

namespace NanamiEngine::Module::Render3D::Shapes
{
    /** @brief 8頂点から立方体を描画する関数 */
    void DrawCube3DFromVertices(const std::array<glm::vec3, 8>& vertices, const int& edgeColor);
    /** @brief カプセルのワイヤーフレーム描画 */
    void DrawCapsule3D(const glm::vec3& center,
                       float radius,
                       float halfHeight,
                       const glm::quat& rotation,
                       const int& color);
    /** @brief 球のワイヤーフレーム描画 */
    void DrawSphere3D(const glm::vec3& center,
                      float radius,
                      const int& color);
    
    /** @brief Meshワイヤーフレーム描画 */
    void DrawMeshWireFrame3D(
        const std::vector<glm::vec3>& vertices,
        const std::vector<uint32_t>&  indices,
        const glm::mat4& world,
        const int& color);
}
