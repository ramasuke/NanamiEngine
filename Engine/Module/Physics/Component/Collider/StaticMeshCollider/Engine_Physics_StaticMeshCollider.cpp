#include "Engine_Physics_StaticMeshCollider.h"

#include "../../../../3DRender/Shapes/Shapes.h"
#include "../../../../Component/ModelRenderer/ModelRenderer.h"
#include "../JoltPhysics/Jolt/Physics/Collision/Shape/MeshShape.h"
#include "../JoltPhysics/Jolt/Geometry/Indexify.h"
#include "../../../../../../Libs/meshoptimizer/meshoptimizer.h"

#include <algorithm>
#include <vector>
#include <DxLib.h>

#include "../../../../GameObject/Transform/Transform.h"
#include "../../../../Log/NanamiEngine_Module_Log.h"

static bool ExtractMeshFromDxModel(
    const int modelHandle,
    JPH::VertexList& outVerts,
    JPH::IndexedTriangleList& outTris)
{
    const int listCount = MV1GetTriangleListNum(modelHandle);
    if (listCount <= 0)
        return false;

    JPH::TriangleList triangles;
    triangles.reserve(20000);

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

            triangles.emplace_back(
                JPH::Float3(vpos[0].x, vpos[0].y, vpos[0].z),
                JPH::Float3(vpos[1].x, vpos[1].y, vpos[1].z),
                JPH::Float3(vpos[2].x, vpos[2].y, vpos[2].z));
        }
    }

    // 隣接三角形の辺を共有させる(アクティブエッジ判定と簡略化に必要)ため、近接頂点を溶接してインデックス化する
    outVerts.clear();
    outTris .clear();
    JPH::Indexify(triangles, outVerts, outTris);

    return !outVerts.empty() && !outTris.empty();
}

static void SimplifyMesh(
    const JPH::VertexList& verts,
    JPH::IndexedTriangleList& ioTris,
    const float maxError,
    const float minTriangleRatio)
{
    std::vector<unsigned int> indices;
    indices.reserve(ioTris.size() * 3);
    for (const auto& tri : ioTris)
        indices.insert(indices.end(), std::begin(tri.mIdx), std::end(tri.mIdx));

    const size_t targetIndexCount = static_cast<size_t>(static_cast<float>(ioTris.size()) * minTriangleRatio) * 3;

    std::vector<unsigned int> simplified(indices.size());
    const size_t indexCount = meshopt_simplify(
        simplified.data(),
        indices.data(),
        indices.size(),
        &verts.front().x,
        verts.size(),
        sizeof(JPH::Float3),
        targetIndexCount,
        maxError,
        meshopt_SimplifyLockBorder | meshopt_SimplifyErrorAbsolute,
        nullptr);

    ioTris.clear();
    ioTris.reserve(indexCount / 3);
    for (size_t i = 0; i + 2 < indexCount; i += 3)
        ioTris.emplace_back(simplified[i], simplified[i + 1], simplified[i + 2], 0);
}

namespace NanamiEngine::Module::Component
{
    void StaticMeshCollider::OnAwake()
    {
        if (!BuildShape())
            return;

        ColliderBase::OnAwake();
    }

    void StaticMeshCollider::OnStart()
    {

    }

    std::pair<JPH::Vec3, JPH::Quat> StaticMeshCollider::CalcWorldTransformInternal() const
    {
        const auto& transform = Transform();

        const glm::quat baseRot  = glm::normalize(transform.GetWorldRot());
        const glm::quat finalRot = glm::normalize(baseRot * glm::quat(glm::radians(offsetRotation_)));

        // メッシュ原点とモデルの見た目のズレを補正する値として配置済みのため、offset_ はワールドスケールを掛けずワールド単位で扱う
        const glm::vec3 worldPos = transform.GetWorldPos() + baseRot * offset_;

        return {
            JPH::Vec3(worldPos.x, worldPos.y, worldPos.z),
            JPH::Quat(finalRot.x, finalRot.y, finalRot.z, finalRot.w)
        };
    }

    bool StaticMeshCollider::BuildShape() const
    {
        shapeBuildAttempted_ = true;

        const auto modelRenderer = Components().Catch<ModelRenderer>().lock();
        if (!modelRenderer || modelRenderer->modelDxLibHandle_ < 0)
        {
            LogError("StaticMeshCollider: ModelRenderer のモデルが見つかりません");
            return false;
        }

        JPH::VertexList verts;
        JPH::IndexedTriangleList tris;
        if (!ExtractMeshFromDxModel(modelRenderer->modelDxLibHandle_, verts, tris))
        {
            LogError("StaticMeshCollider: モデルからメッシュを取得できません");
            return false;
        }

        const size_t sourceTriangleCount = tris.size();

        // 簡略化の誤差をワールド単位で扱うため、スケールは簡略化より先に焼き込む
        const glm::vec3 scale = Transform().GetWorldScale() * scale_;
        for (auto& v : verts)
        {
            v = JPH::Float3(v.x * scale.x, v.y * scale.y, v.z * scale.z);
        }

        if (simplifyEnabled_)
        {
            SimplifyMesh(verts, tris, std::max(maxSimplifyError_, 0.0f), std::clamp(minTriangleRatio_, 0.0f, 1.0f));
        }

        const JPH::MeshShapeSettings settings(std::move(verts), std::move(tris));
        const auto result = settings.Create();
        if (result.HasError())
        {
            LogError(std::string("StaticMeshCollider: MeshShape の作成に失敗しました: ") + result.GetError().c_str());
            return false;
        }

        shape_               = result.Get();
        sourceTriangleCount_ = sourceTriangleCount;
        shapeTriangleCount_  = settings.mIndexedTriangles.size();
        return true;
    }

    void StaticMeshCollider::OnDebugDraw() const
    {
        if (!shapeBuildAttempted_ && !BuildShape())
            return;

        if (!shape_)
            return;

        // Compound では重心と原点がずれるので、重心ではなく形状の原点の位置で描く
        const auto [position, rotation] = SimulatedWorldTransform().value_or(CalcWorldTransformInternal());

        JPH::Shape::GetTrianglesContext context;
        shape_->GetTrianglesStart(
            context,
            JPH::AABox::sBiggest(),
            position,
            rotation,
            JPH::Vec3::sOne());

        const unsigned int color = DebugDrawColor(GetColor(0, 255, 0));
        JPH::Float3 vertices[JPH::Shape::cGetTrianglesMinTrianglesRequested * 3];

        int count;
        while ((count = shape_->GetTrianglesNext(context, JPH::Shape::cGetTrianglesMinTrianglesRequested, vertices)) > 0)
        {
            for (int i = 0; i < count; i++)
            {
                const JPH::Float3& v0 = vertices[i * 3];
                const JPH::Float3& v1 = vertices[i * 3 + 1];
                const JPH::Float3& v2 = vertices[i * 3 + 2];

                const VECTOR p0 = VGet(v0.x, v0.y, v0.z);
                const VECTOR p1 = VGet(v1.x, v1.y, v1.z);
                const VECTOR p2 = VGet(v2.x, v2.y, v2.z);

                DrawLine3D(p0, p1, color);
                DrawLine3D(p1, p2, color);
                DrawLine3D(p2, p0, color);
            }
        }
    }

    JPH::RefConst<JPH::Shape> StaticMeshCollider::CreateColliderShape() const
    {
        return shape_;
    }

    void StaticMeshCollider::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_", offset_);
        ImGuiHelper::OnDrawInputField("scale_", scale_);
        ImGuiHelper::OnDrawInputField("offsetRotation_", offsetRotation_);
        int layerIndex = Physics::ToIndex(layer_);
        if (ImGui::Combo("Layer", &layerIndex, Physics::LAYER_NAMES, static_cast<int>(Physics::Layer::Count)))
        {
            layer_ = Physics::ToLayer(layerIndex);
        }
        ImGuiHelper::OnDrawInputField("simplifyEnabled_", simplifyEnabled_);
        if (simplifyEnabled_)
        {
            ImGuiHelper::OnDrawInputField("maxSimplifyError_", maxSimplifyError_);
            ImGuiHelper::OnDrawInputField("minTriangleRatio_", minTriangleRatio_);
        }
        ImGui::Text("Triangles: %zu -> %zu", sourceTriangleCount_, shapeTriangleCount_);
        if (ImGui::Button("Rebuild") && BuildShape())
        {
            NotifyShapeChanged();
        }
        OnDebugDraw();
    }
}
