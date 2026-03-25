#define GLM_ENABLE_EXPERIMENTAL 
#include "Shapes.h"
#include "DxLib.h"
#include "gtc/constants.hpp"
#include "gtx/quaternion.hpp"

void NanamiEngine::Module::Render3D::Shapes::DrawCube3DFromVertices(const std::array<glm::vec3, 8>& vertices,
                                                                    const int& edgeColor)
{
    // 立方体の辺を構成するインデックスペア
    static const int EDGES[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // 下の面
        {4,5}, {5,6}, {6,7}, {7,4}, // 上の面
        {0,4}, {1,5}, {2,6}, {3,7}  // 側面
    };

    for (auto& e : EDGES)
    {
        const glm::vec3& p1 = vertices[e[0]];
        const glm::vec3& p2 = vertices[e[1]];
        DrawLine3D({p1.x, p1.y, p1.z}, {p2.x, p2.y, p2.z}, edgeColor);
    }
}

static glm::vec3 RotatePoint(const glm::vec3& p, const glm::quat& q)
{
    return q * p;
}

void NanamiEngine::Module::Render3D::Shapes::DrawCapsule3D(
    const glm::vec3& center,
    float radius,
    float halfHeight,
    const glm::quat& rotation,
    const int& color)
{
    constexpr int segment = 16;
    // 円柱部分の上下円
    std::array<glm::vec3, segment> bottom{};
    std::array<glm::vec3, segment> top{};

    for (int i = 0; i < segment; i++)
    {
        float t = static_cast<float>(i) / segment * glm::two_pi<float>();
        float x = std::cos(t) * radius;
        float z = std::sin(t) * radius;

        glm::vec3 b  = RotatePoint(glm::vec3(x, -halfHeight, z), rotation) + center;
        glm::vec3 t0 = RotatePoint(glm::vec3(x,  halfHeight, z), rotation) + center;

        bottom[i] = b;
        top[i] = t0;
    }

    // 円柱側面 + 上下の円周
    for (int i = 0; i < segment; i++)
    {
        int n = (i + 1) % segment;

        DrawLine3D(VGet(bottom[i].x, bottom[i].y, bottom[i].z),
                   VGet(bottom[n].x, bottom[n].y, bottom[n].z), color);

        DrawLine3D(VGet(top[i].x, top[i].y, top[i].z),
                   VGet(top[n].x, top[n].y, top[n].z), color);

        DrawLine3D(VGet(bottom[i].x, bottom[i].y, bottom[i].z),
                   VGet(top[i].x, top[i].y, top[i].z), color);
    }

    // 半球（上下）
    for (int i = 0; i < segment; i += 2)  // ゴチャつくので 2 本に 1 本だけ経線
    {
        constexpr int hemiSeg = 8;
        float t = static_cast<float>(i) / segment * glm::two_pi<float>();
        float dx = std::cos(t);
        float dz = std::sin(t);

        for (int j = 0; j < hemiSeg; j++)
        {
            float a0 = static_cast<float>(j) / hemiSeg * glm::half_pi<float>();
            float a1 = static_cast<float>(j + 1) / hemiSeg * glm::half_pi<float>();

            // ---- 下半球 ----
            {
                auto p0 = glm::vec3(dx * radius * std::cos(a0),
                                          -halfHeight - radius * std::sin(a0),
                                          dz * radius * std::cos(a0));

                auto p1 = glm::vec3(dx * radius * std::cos(a1),
                                          -halfHeight - radius * std::sin(a1),
                                          dz * radius * std::cos(a1));

                p0 = RotatePoint(p0, rotation) + center;
                p1 = RotatePoint(p1, rotation) + center;

                DrawLine3D(VGet(p0.x, p0.y, p0.z),
                           VGet(p1.x, p1.y, p1.z), color);
            }

            // ---- 上半球 ----
            {
                auto p0 = glm::vec3(dx * radius * std::cos(a0),
                                          halfHeight + radius * std::sin(a0),
                                          dz * radius * std::cos(a0));

                auto p1 = glm::vec3(dx * radius * std::cos(a1),
                                          halfHeight + radius * std::sin(a1),
                                          dz * radius * std::cos(a1));

                p0 = RotatePoint(p0, rotation) + center;
                p1 = RotatePoint(p1, rotation) + center;

                DrawLine3D(VGet(p0.x, p0.y, p0.z), VGet(p1.x, p1.y, p1.z), color);
            }
        }
    }
}

void NanamiEngine::Module::Render3D::Shapes::DrawMeshWireFrame3D(
    const std::vector<glm::vec3>& vertices,
    const std::vector<uint32_t>&  indices,
    const glm::mat4& world,
    const int& color)
{
    if (vertices.empty() || indices.empty())
        return;

    // 三角形数検証
    assert(indices.size() % 3 == 0);

    auto transformPoint = [&](const glm::vec3& v)
    {
        const glm::vec4 wp = world * glm::vec4(v, 1.0f);
        return glm::vec3(wp);
    };

    const size_t triCount = indices.size() / 3;

    for (size_t t = 0; t < triCount; t++)
    {
        const uint32_t i0 = indices[t * 3 + 0];
        const uint32_t i1 = indices[t * 3 + 1];
        const uint32_t i2 = indices[t * 3 + 2];

        if (i0 >= vertices.size()) continue;
        if (i1 >= vertices.size()) continue;
        if (i2 >= vertices.size()) continue;

        glm::vec3 p0 = transformPoint(vertices[i0]);
        glm::vec3 p1 = transformPoint(vertices[i1]);
        glm::vec3 p2 = transformPoint(vertices[i2]);

        DrawLine3D(VGet(p0.x, p0.y, p0.z),
                   VGet(p1.x, p1.y, p1.z), color);

        DrawLine3D(VGet(p1.x, p1.y, p1.z),
                   VGet(p2.x, p2.y, p2.z), color);

        DrawLine3D(VGet(p2.x, p2.y, p2.z),
                   VGet(p0.x, p0.y, p0.z), color);
    }
}
