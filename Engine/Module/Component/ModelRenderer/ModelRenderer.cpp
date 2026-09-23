#include "ModelRenderer.h"
#include "../../../Core/Coroutine/Coroutine.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include <../../Libs/glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        bool IsRigidVertexType(const int vertexType)
        {
            switch (vertexType)
            {
            case DX_MV1_VERTEX_TYPE_1FRAME:
            case DX_MV1_VERTEX_TYPE_FREE_FRAME:
            case DX_MV1_VERTEX_TYPE_NMAP_1FRAME:
            case DX_MV1_VERTEX_TYPE_NMAP_FREE_FRAME:
                return true;
            default:
                return false;
            }
        }
    }

    void ModelRenderer::InitRenderer()
    {
        ReloadModel();
    }

    void ModelRenderer::ReloadModel()
    {
        if (modelDxLibHandle_ != -1)
        {
            MV1DeleteModel(modelDxLibHandle_);
            modelDxLibHandle_ = -1;
        }
        if (mv1File_)
            modelDxLibHandle_ = mv1File_->LoadDxLibHandle();

        RefreshTriangleListInfo();
    }

    void ModelRenderer::SetMv1File(const std::shared_ptr<Asset::Mv1File>& mv1File)
    {
        mv1File_ = mv1File;
        ReloadModel();
    }

    void ModelRenderer::RefreshTriangleListInfo()
    {
        rigidTriangleList_       .clear();
        materialNames_           .clear();
        originalMaterialBlend_   .clear();
        triangleListMaterialIndex_.clear();
        meshMaterialIndex_       .clear();
        meshOriginalCulling_     .clear();
        materialPasses_          .clear();
        materialPassActive_      .clear();
        allRigid_             = true;
        materialStateApplied_ = false;

        if (modelDxLibHandle_ == -1)
            return;

        const int listNum = MV1GetTriangleListNum(modelDxLibHandle_);
        rigidTriangleList_.reserve(listNum);
        for (int i = 0; i < listNum; ++i)
        {
            const bool rigid = IsRigidVertexType(MV1GetTriangleListVertexType(modelDxLibHandle_, i));
            rigidTriangleList_.push_back(rigid);
            allRigid_ = allRigid_ && rigid;
        }

        const int materialNum = MV1GetMaterialNum(modelDxLibHandle_);
        materialNames_        .reserve(materialNum);
        originalMaterialBlend_.reserve(materialNum);
        for (int i = 0; i < materialNum; ++i)
        {
            materialNames_.emplace_back(MV1GetMaterialName(modelDxLibHandle_, i));
            originalMaterialBlend_.emplace_back(
                MV1GetMaterialDrawBlendMode (modelDxLibHandle_, i),
                MV1GetMaterialDrawBlendParam(modelDxLibHandle_, i));
        }

        // トライアングルリストから材質を直接引く API が無いので、メッシュ経由で対応表を作る
        triangleListMaterialIndex_.assign(listNum, -1);
        const int meshNum = MV1GetMeshNum(modelDxLibHandle_);
        meshMaterialIndex_  .reserve(meshNum);
        meshOriginalCulling_.reserve(meshNum);
        for (int mesh = 0; mesh < meshNum; ++mesh)
        {
            const int materialIndex = MV1GetMeshMaterial(modelDxLibHandle_, mesh);
            meshMaterialIndex_  .push_back(materialIndex);
            meshOriginalCulling_.push_back(MV1GetMeshBackCulling(modelDxLibHandle_, mesh));

            const int meshListNum = MV1GetMeshTListNum(modelDxLibHandle_, mesh);
            for (int i = 0; i < meshListNum; ++i)
            {
                const int listIndex = MV1GetMeshTList(modelDxLibHandle_, mesh, i);
                if (listIndex >= 0 && listIndex < listNum)
                    triangleListMaterialIndex_[listIndex] = materialIndex;
            }
        }
    }

    void ModelRenderer::OnPreFixedUpdate()
    {
        if (!useFixedInterpolation_)
            return;

        if (hasCurrCapture_)
        {
            prevWorldPos_ = currWorldPos_;
            prevWorldRot_ = currWorldRot_;
            hasPrevCapture_ = true;
        }
        else
        {
            prevWorldPos_   = Transform().GetWorldPos();
            prevWorldRot_   = Transform().GetWorldRot();
            hasPrevCapture_ = true;
        }
    }

    void ModelRenderer::OnUpdatedPhysics()
    {
        if (!useFixedInterpolation_)
            return;

        currWorldPos_   = Transform().GetWorldPos();
        currWorldRot_   = Transform().GetWorldRot();
        hasCurrCapture_ = true;

        if (!hasPrevCapture_)
        {
            prevWorldPos_   = currWorldPos_;
            prevWorldRot_   = currWorldRot_;
            hasPrevCapture_ = true;
        }
    }

    bool ModelRenderer::IsInterpolating() const
    {
        return useFixedInterpolation_ && hasPrevCapture_ && hasCurrCapture_;
    }

    glm::vec3 ModelRenderer::RenderWorldPos() const
    {
        if (!IsInterpolating())
            return Transform().GetWorldPos();

        return glm::mix(prevWorldPos_, currWorldPos_, Time::GetFixedAlpha());
    }

    glm::mat4 ModelRenderer::GetRenderMatrix() const
    {
        glm::mat4 matrix;
        if (IsInterpolating())
        {
            const float alpha     = Time::GetFixedAlpha();
            const glm::vec3 pos   = RenderWorldPos();
            const glm::quat rot   = glm::slerp(prevWorldRot_, currWorldRot_, alpha);
            const glm::vec3 scale = Transform().GetWorldScale();
            const glm::mat4 mat   = glm::translate(glm::mat4(1.0f), pos)
                                  * glm::mat4_cast(rot)
                                  * glm::scale(glm::mat4(1.0f), scale);
            matrix = mat;
        }
        else
        {
            matrix = Transform().GetWorldMatrix();
        }

        matrix[3][0] += renderOffset_.x;
        matrix[3][1] += renderOffset_.y;
        matrix[3][2] += renderOffset_.z;
        return matrix;
    }

    void ModelRenderer::ResolveMaterialPasses(const PolicyList& policies)
    {
        const int materialNum = static_cast<int>(materialNames_.size());
        materialPasses_    .assign(materialNum, MaterialShaderPass{});
        materialPassActive_.assign(materialNum, false);

        bool anyDisableZWrite = false;
        for (int i = 0; i < materialNum; ++i)
        {
            MaterialShaderPass resolved{};
            bool matched = false;
            for (const auto& weakPolicy : policies)
            {
                const auto policy = weakPolicy.lock();
                if (!policy)
                    continue;

                MaterialShaderPass pass{};
                if (policy->TryGetMaterialShaderPass(materialNames_[i], pass))
                {
                    resolved = pass;
                    matched  = true;
                    break;
                }
            }

            materialPasses_[i]     = resolved;
            materialPassActive_[i] = matched;

            // SetDrawBlendMode / SetWriteZBuffer3D はモデル描画には反映されないため、MV1 専用の API を使う
            if (matched)
            {
                anyDisableZWrite = anyDisableZWrite || resolved.disableZWrite;
                MV1SetMaterialDrawBlendMode (modelDxLibHandle_, i, static_cast<int>(resolved.blendMode));
                MV1SetMaterialDrawBlendParam(modelDxLibHandle_, i, resolved.blendParam);
            }
            else
            {
                MV1SetMaterialDrawBlendMode (modelDxLibHandle_, i, originalMaterialBlend_[i].first);
                MV1SetMaterialDrawBlendParam(modelDxLibHandle_, i, originalMaterialBlend_[i].second);
            }
        }

        MV1SetWriteZBuffer(modelDxLibHandle_, anyDisableZWrite ? FALSE : TRUE);

        // 両面描画はメッシュ単位の API しか無いので、対象材質を使うメッシュへ適用する
        const int meshNum = static_cast<int>(meshMaterialIndex_.size());
        for (int mesh = 0; mesh < meshNum; ++mesh)
        {
            const int  materialIndex  = meshMaterialIndex_[mesh];
            const bool disableCulling = materialIndex >= 0
                                     && materialPassActive_[materialIndex]
                                     && materialPasses_[materialIndex].disableCulling;
            MV1SetMeshBackCulling(modelDxLibHandle_, mesh,
                                  disableCulling ? DX_CULLING_NONE : meshOriginalCulling_[mesh]);
        }

        materialStateApplied_ = true;
    }

    void ModelRenderer::RestoreDefaultMaterialState()
    {
        if (!materialStateApplied_)
            return;

        const int materialNum = static_cast<int>(originalMaterialBlend_.size());
        for (int i = 0; i < materialNum; ++i)
        {
            MV1SetMaterialDrawBlendMode (modelDxLibHandle_, i, originalMaterialBlend_[i].first);
            MV1SetMaterialDrawBlendParam(modelDxLibHandle_, i, originalMaterialBlend_[i].second);
        }

        const int meshNum = static_cast<int>(meshOriginalCulling_.size());
        for (int mesh = 0; mesh < meshNum; ++mesh)
            MV1SetMeshBackCulling(modelDxLibHandle_, mesh, meshOriginalCulling_[mesh]);

        MV1SetWriteZBuffer(modelDxLibHandle_, TRUE);
        materialStateApplied_ = false;
    }

    void ModelRenderer::DrawWithMaterialPolicies()
    {
        int currentVs = -1;
        int currentPs = -1;
        int currentCb = -1;

        const int listNum = static_cast<int>(triangleListMaterialIndex_.size());
        for (int i = 0; i < listNum; ++i)
        {
            const int  materialIndex = triangleListMaterialIndex_[i];
            const bool useCustom     = rigidTriangleList_[i]
                                    && materialIndex >= 0
                                    && materialPassActive_[materialIndex];
            if (useCustom)
            {
                const MaterialShaderPass& pass = materialPasses_[materialIndex];
                if (pass.vsHandle != currentVs)
                {
                    SetUseVertexShader(pass.vsHandle);
                    currentVs = pass.vsHandle;
                }
                if (pass.psHandle != currentPs)
                {
                    SetUsePixelShader(pass.psHandle);
                    currentPs = pass.psHandle;
                }
                if (pass.cbHandle != currentCb)
                {
                    SetShaderConstantBuffer(pass.cbHandle, DX_SHADERTYPE_VERTEX, CUSTOM_SHADER_CB_SLOT);
                    SetShaderConstantBuffer(pass.cbHandle, DX_SHADERTYPE_PIXEL,  CUSTOM_SHADER_CB_SLOT);
                    currentCb = pass.cbHandle;
                }
            }
            MV1SetUseOrigShader(useCustom ? TRUE : FALSE);
            MV1DrawTriangleList(modelDxLibHandle_, i);
        }

        MV1SetUseOrigShader(FALSE);
        SetUseVertexShader(-1);
        SetUsePixelShader (-1);
    }

    bool ModelRenderer::ShouldDrawShadowForMaterial(const PolicyList& policies, const std::string& materialName) const
    {
        for (const auto& weakPolicy : policies)
        {
            const auto policy = weakPolicy.lock();
            if (policy && !policy->ShouldDrawShadow(materialName))
                return false;
        }
        return true;
    }

    void ModelRenderer::OnShadowRender()
    {
        if (!IsEnable() || modelDxLibHandle_ == -1)
            return;

        MV1SetMatrix(modelDxLibHandle_, LibCore::Dxlib::ToDxMatrix(GetRenderMatrix()));

        // 影は常に標準シェーダー
        const PolicyList policies = Components().Catches<IModelMaterialShaderPolicy>();
        if (policies.empty())
        {
            MV1DrawModel(modelDxLibHandle_);
            return;
        }

        MV1SetUseOrigShader(FALSE);
        const int listNum = static_cast<int>(triangleListMaterialIndex_.size());
        for (int i = 0; i < listNum; ++i)
        {
            const int materialIndex = triangleListMaterialIndex_[i];
            if (materialIndex >= 0 && !ShouldDrawShadowForMaterial(policies, materialNames_[materialIndex]))
                continue;
            MV1DrawTriangleList(modelDxLibHandle_, i);
        }
    }

    void ModelRenderer::OnRender()
    {
        if (!IsEnable() || modelDxLibHandle_ == -1)
            return;

        MV1SetMatrix(modelDxLibHandle_, LibCore::Dxlib::ToDxMatrix(GetRenderMatrix()));

        const PolicyList policies = Components().Catches<IModelMaterialShaderPolicy>();
        if (policies.empty())
        {
            RestoreDefaultMaterialState();
            MV1DrawModel(modelDxLibHandle_);
            return;
        }

        ResolveMaterialPasses(policies);
        DrawWithMaterialPolicies();
    }

    void ModelRenderer::OnDestroy()
    {
        if (modelDxLibHandle_ != -1)
            MV1DeleteModel(modelDxLibHandle_);
    }

    void ModelRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("mv1File_",               mv1File_);
        ImGuiHelper::OnDrawInputField("useFixedInterpolation_", useFixedInterpolation_);
        if (ImGui::Button("OnUpdateDxLibHandle"))
        {
            ReloadModel();
        }
        ImGui::Text("triangleLists: %d  allRigid: %s",
                    static_cast<int>(rigidTriangleList_.size()), allRigid_ ? "true" : "false");
        // 材質名は IModelMaterialShaderPolicy 側の指定に使うので見えるようにしておく
        for (int i = 0; i < static_cast<int>(materialNames_.size()); ++i)
        {
            const bool active = i < static_cast<int>(materialPassActive_.size()) && materialPassActive_[i];
            ImGui::Text("material[%d]: %s%s", i, materialNames_[i].c_str(), active ? "  (policy)" : "");
        }
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::ModelRenderer);
#pragma endregion
