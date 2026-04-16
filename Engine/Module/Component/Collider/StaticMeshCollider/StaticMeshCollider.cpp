#include "StaticMeshCollider.h"

#include "../../../../Core/Physics/Physics.h"
#include "../../../3DRender/Shapes/Shapes.h"
#include "../../ModelRenderer/ModelRenderer.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyInterface.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyCreationSettings.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/MeshShape.h"

#include <unordered_map>
#include <DxLib.h>

#include "../../../GameObject/Transform/Transform.h"

static bool ExtractMeshFromDxModel(
    const int modelHandle,
    JPH::VertexList& outVerts,
    JPH::IndexedTriangleList& outTris)
{
    const int listCount = MV1GetTriangleListNum(modelHandle);
    if (listCount <= 0)
        return false;

    outVerts.clear();
    outTris .clear();
    outVerts.reserve(20000);
    outTris .reserve(20000);

    std::unordered_map<long long, uint32_t> vertexCache;
    vertexCache.reserve(20000);

    uint32_t runningIndex = 0;

    // トライアングルリストを走査
    for (int tl = 0; tl < listCount; tl++)
    {
        const int polyCount = MV1GetTriangleListPolygonNum(modelHandle, tl);
        if (polyCount <= 0) continue;

        // ポリゴンを走査
        for (int p = 0; p < polyCount; p++)
        {
            VECTOR vpos[3];
            int vnum =
                MV1GetTriangleListPolygonVertexPosition(
                    modelHandle,
                    tl,
                    p,
                    vpos,
                    nullptr);

            if (vnum != 3)
                continue;

            JPH::IndexedTriangle tri;

            // 頂点作成 & 共有化
            for (int i = 0; i < 3; i++)
            {
                long long key =
                      static_cast<long long>(vpos[i].x * 10000)      & 0x1FFFFF
                    | static_cast<long long>(vpos[i].y * 10000) << 21 & 0x3FFFFFULL
                    | static_cast<long long>(vpos[i].z * 10000) << 42;

                if (auto it = vertexCache.find(key); it != vertexCache.end())
                {
                    tri.mIdx[i] = it->second;
                    continue;
                }

                outVerts.emplace_back(vpos[i].x, vpos[i].y, vpos[i].z);

                tri.mIdx[i] = runningIndex;
                vertexCache[key] = runningIndex;
                runningIndex++;
            }

            tri.mMaterialIndex = 0;

            outTris.push_back(tri);
        }
    }

    return !outVerts.empty() && !outTris.empty();
}

namespace NanamiEngine::Module::Component
{
    void StaticMeshCollider::OnStart()
    {
        const auto modelRenderer = Components().Catch<ModelRenderer>().lock();
        if (!modelRenderer)
            return;

        const int modelHandle = modelRenderer->modelDxLibHandle_;
        if (modelHandle < 0)
            return;

        JPH::VertexList verts;
        JPH::IndexedTriangleList tris;

        if (!ExtractMeshFromDxModel(modelHandle, verts, tris))
        {
            printf("StaticMeshCollider failed : no mesh\n");
            return;
        }
        
        const glm::vec3 scale = Transform().GetWorldScale() * scale_;
        for (auto& v : verts)
        {
            v = JPH::Float3(
                v.x * scale.x,
                v.y * scale.y,
                v.z * scale.z
            );
        }


        // MeshShapeを生成
        const JPH::MeshShapeSettings settings(verts, tris);
        const auto result = settings.Create();

        if (result.HasError())
        {
            printf("MeshCollider FAILED: %s\n", result.GetError().c_str());
            return;
        }

        // Collider 生成
        const glm::vec3 world   = Transform().GetWorldPos() + offset_;
        const glm::quat rawRot  = Transform().GetWorldRot();
        const glm::quat normalizedRot = glm::normalize(rawRot);
        bodyId_ = Core::Application::ApplicationBase::Physics().CreateCollider(
            result.Get(),
            JPH::Vec3(world.x, world.y, world.z),
            JPH::Quat(normalizedRot.x, normalizedRot.y, normalizedRot.z, normalizedRot.w),
            JPH::EMotionType::Static,
            0,
            false,
            IsGravity(),
            &Components());
    }

    void StaticMeshCollider::OnDebugDraw() const
    {
        const auto modelRenderer = Components().Catch<ModelRenderer>().lock();
        if (!modelRenderer)
            return;

        const int model = modelRenderer->modelDxLibHandle_;
        if (model < 0)
            return;

        JPH::VertexList verts;
        JPH::IndexedTriangleList tris;

        if (!ExtractMeshFromDxModel(model, verts, tris))
        {
            printf("StaticMeshCollider failed : no mesh\n");
            return;
        }

        const glm::vec3 scale = Transform().GetWorldScale() * scale_;
        for (auto& v : verts)
        {
            v = JPH::Float3(
                v.x * scale.x,
                v.y * scale.y,
                v.z * scale.z
            );
        }

        if (verts.empty() || tris.empty())
            return;

        const glm::vec3 worldPos = Transform().GetWorldPos() + offset_;
        const auto rawRot = Transform().GetWorldRot();
        const glm::quat q        = glm::normalize(rawRot);

        // Transform matrix
        glm::mat4 m = glm::mat4_cast(q);
        m[3][0] = worldPos.x;
        m[3][1] = worldPos.y;
        m[3][2] = worldPos.z;

        auto xf = [&](const JPH::Vec3& v)->VECTOR {
            glm::vec4 p(v.GetX(), v.GetY(), v.GetZ(), 1.0f);
            p = m * p;
            return VGet(p.x, p.y, p.z);
        };
        
        // draw mesh
        for (const auto& tri : tris)
        {
            const JPH::Vec3 v0(
                verts[tri.mIdx[0]].x,
                verts[tri.mIdx[0]].y,
                verts[tri.mIdx[0]].z
            );

            const JPH::Vec3 v1(
                verts[tri.mIdx[1]].x,
                verts[tri.mIdx[1]].y,
                verts[tri.mIdx[1]].z
            );

            const JPH::Vec3 v2(
                verts[tri.mIdx[2]].x,
                verts[tri.mIdx[2]].y,
                verts[tri.mIdx[2]].z
            );

            const VECTOR p0 = xf(v0);
            const VECTOR p1 = xf(v1);
            const VECTOR p2 = xf(v2);

            DrawLine3D(p0, p1, GetColor(0,255,0));
            DrawLine3D(p1, p2, GetColor(0,255,0));
            DrawLine3D(p2, p0, GetColor(0,255,0));
        }
    }

    JPH::RefConst<JPH::Shape> StaticMeshCollider::CreateColliderShape() const
    {
        const auto modelRenderer = Components().Catch<ModelRenderer>().lock();
        if (!modelRenderer)
            throw std::exception("error");

        const int modelHandle = modelRenderer->modelDxLibHandle_;
        if (modelHandle < 0)
            throw std::exception("error");
        
        JPH::VertexList verts;
        JPH::IndexedTriangleList tris;

        if (!ExtractMeshFromDxModel(modelHandle, verts, tris))
        {
            printf("StaticMeshCollider failed : no mesh\n");
            throw std::exception("error");
        }
        
        const glm::vec3 scale = Transform().GetWorldScale() * scale_;
        for (auto& v : verts)
        {
            v = JPH::Float3(
                v.x * scale.x,
                v.y * scale.y,
                v.z * scale.z
            );
        }


        // MeshShapeを生成
        const JPH::MeshShapeSettings settings(verts, tris);
        const auto result = settings.Create();

        // Collider 生成
        const glm::vec3 world   = Transform().GetWorldPos() + offset_;
        const glm::quat rawRot  = Transform().GetWorldRot();
        const glm::quat normalizedRot = glm::normalize(rawRot);
        return result.Get();
    }

    void StaticMeshCollider::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_", offset_);
        ImGuiHelper::OnDrawInputField("scale_", scale_);
        ImGuiHelper::OnDrawInputField("rotation", rotation_);
        int layerIndex = Physics::ToIndex(layer_);
        if (ImGui::Combo("Layer", &layerIndex, Physics::LAYER_NAMES, static_cast<int>(Physics::Layer::Count)))
        {
            layer_ = Physics::ToLayer(layerIndex);
        }
        OnDebugDraw();
    }
}
