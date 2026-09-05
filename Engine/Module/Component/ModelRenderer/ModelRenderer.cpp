#include "ModelRenderer.h"
#include "../../../Core/Coroutine/Coroutine.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include <../../Libs/glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace NanamiEngine::Module::Component
{
    namespace
    {
        // DxLib(DX11) はトライアングルリストの頂点タイプごとに頂点レイアウトが異なる。
        // 剛体メッシュ用の頂点シェーダーで描画できるのはボーン情報を持たないタイプのみ。
        // (FREE_FRAME は DxLib 側で CPU スキニングされ、剛体メッシュとして描画される)
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
        if (mv1File_)
            modelDxLibHandle_ = mv1File_->LoadDxLibHandle();

        RefreshTriangleListInfo();
    }

    void ModelRenderer::RefreshTriangleListInfo()
    {
        rigidTriangleList_.clear();
        originalMaterialBlend_.clear();
        allRigid_           = true;
        customStateApplied_ = false; // 新しいハンドルはデフォルト状態

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
    }

    int ModelRenderer::GetOrCreateShaderConstantBufferHandle()
    {
        if (!HasCustomShader())
            return -1;

        if (cbHandle_ == -1)
            cbHandle_ = CreateShaderConstantBuffer(CUSTOM_SHADER_CB_SIZE);

        return cbHandle_;
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

    static MATRIX GlmMatToDxMat(const glm::mat4& m)
    {
        MATRIX d;
        d.m[0][0]=m[0][0]; d.m[0][1]=m[0][1]; d.m[0][2]=m[0][2]; d.m[0][3]=m[0][3];
        d.m[1][0]=m[1][0]; d.m[1][1]=m[1][1]; d.m[1][2]=m[1][2]; d.m[1][3]=m[1][3];
        d.m[2][0]=m[2][0]; d.m[2][1]=m[2][1]; d.m[2][2]=m[2][2]; d.m[2][3]=m[2][3];
        d.m[3][0]=m[3][0]; d.m[3][1]=m[3][1]; d.m[3][2]=m[3][2]; d.m[3][3]=m[3][3];
        return d;
    }

    MATRIX ModelRenderer::GetRenderMatrix() const
    {
        if (useFixedInterpolation_ && hasPrevCapture_ && hasCurrCapture_)
        {
            const float alpha     = Time::GetFixedAlpha();
            const glm::vec3 pos   = glm::mix  (prevWorldPos_, currWorldPos_, alpha);
            const glm::quat rot   = glm::slerp(prevWorldRot_, currWorldRot_, alpha);
            const glm::vec3 scale = Transform().GetWorldScale();
            const glm::mat4 mat   = glm::translate(glm::mat4(1.0f), pos)
                                  * glm::mat4_cast(rot)
                                  * glm::scale(glm::mat4(1.0f), scale);
            return GlmMatToDxMat(mat);
        }
        return Transform().GetDxWorldMatrix();
    }

    void ModelRenderer::OnShadowRender()
    {
        if (!IsEnable() || modelDxLibHandle_ == -1)
            return;

        // カスタムシェーダーが設定されている場合はシャドウをスキップ
        // （透明度制御がシェーダー側にあるため、影だけ落ちる状態を防ぐ）
        if (HasCustomShader())
            return;

        MV1SetMatrix(modelDxLibHandle_, GetRenderMatrix());
        MV1DrawModel(modelDxLibHandle_);
    }

    bool ModelRenderer::HasCustomShader() const
    {
        return vsFile_ && psFile_
            && vsFile_->GetVsHandle() != -1
            && psFile_->GetPsHandle() != -1;
    }

    void ModelRenderer::ApplyCustomModelState()
    {
        if (customStateApplied_)
            return;

        // 復元用に元のマテリアルのブレンド設定を退避
        const int materialNum = MV1GetMaterialNum(modelDxLibHandle_);
        originalMaterialBlend_.clear();
        originalMaterialBlend_.reserve(materialNum);
        for (int i = 0; i < materialNum; ++i)
        {
            originalMaterialBlend_.emplace_back(
                MV1GetMaterialDrawBlendMode (modelDxLibHandle_, i),
                MV1GetMaterialDrawBlendParam(modelDxLibHandle_, i));
        }

        // SetDrawBlendMode / SetWriteZBuffer3D はモデル描画には反映されないため、
        // MV1 専用の API でアルファブレンド + Z 書き込み無しにする
        MV1SetMaterialDrawBlendModeAll (modelDxLibHandle_, DX_BLENDMODE_ALPHA);
        MV1SetMaterialDrawBlendParamAll(modelDxLibHandle_, 255);
        MV1SetWriteZBuffer             (modelDxLibHandle_, FALSE);
        customStateApplied_ = true;
    }

    void ModelRenderer::RestoreDefaultModelState()
    {
        if (!customStateApplied_)
            return;

        const int materialNum = static_cast<int>(originalMaterialBlend_.size());
        for (int i = 0; i < materialNum; ++i)
        {
            MV1SetMaterialDrawBlendMode (modelDxLibHandle_, i, originalMaterialBlend_[i].first);
            MV1SetMaterialDrawBlendParam(modelDxLibHandle_, i, originalMaterialBlend_[i].second);
        }
        MV1SetWriteZBuffer(modelDxLibHandle_, TRUE);
        customStateApplied_ = false;
    }

    void ModelRenderer::DrawWithCustomShader()
    {
        ApplyCustomModelState();

        SetUseVertexShader(vsFile_->GetVsHandle());
        SetUsePixelShader (psFile_->GetPsHandle());
        if (cbHandle_ != -1)
        {
            SetShaderConstantBuffer(cbHandle_, DX_SHADERTYPE_VERTEX, CUSTOM_SHADER_CB_SLOT);
            SetShaderConstantBuffer(cbHandle_, DX_SHADERTYPE_PIXEL,  CUSTOM_SHADER_CB_SLOT);
        }
        MV1SetUseOrigShader(TRUE);

        if (allRigid_)
        {
            MV1DrawModel(modelDxLibHandle_);
        }
        else
        {
            // 4/8 ボーンのスキンメッシュは剛体用頂点シェーダーでは描画できないため、
            // そのトライアングルリストだけ DxLib 標準シェーダーで描画する(フェードは掛からない)
            const int listNum = (std::min)(MV1GetTriangleListNum(modelDxLibHandle_),
                                           static_cast<int>(rigidTriangleList_.size()));
            for (int i = 0; i < listNum; ++i)
            {
                MV1SetUseOrigShader(rigidTriangleList_[i] ? TRUE : FALSE);
                MV1DrawTriangleList(modelDxLibHandle_, i);
            }
        }

        MV1SetUseOrigShader(FALSE);
        SetUseVertexShader(-1);
        SetUsePixelShader (-1);
    }

    void ModelRenderer::OnRender()
    {
        if (!IsEnable() || modelDxLibHandle_ == -1)
            return;

        MV1SetMatrix(modelDxLibHandle_, GetRenderMatrix());

        if (!HasCustomShader())
        {
            RestoreDefaultModelState();
            MV1DrawModel(modelDxLibHandle_);
            return;
        }

        GetOrCreateShaderConstantBufferHandle();
        DrawWithCustomShader();
    }

    void ModelRenderer::OnDestroy()
    {
        if (modelDxLibHandle_ != -1)
            MV1DeleteModel(modelDxLibHandle_);
        if (cbHandle_ != -1)
            DeleteShaderConstantBuffer(cbHandle_);
    }

    void ModelRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("mv1File_",              mv1File_);
        ImGuiHelper::OnDrawInputField("vsFile_",               vsFile_);
        ImGuiHelper::OnDrawInputField("psFile_",               psFile_);
        ImGuiHelper::OnDrawInputField("useFixedInterpolation_", useFixedInterpolation_);
        if (ImGui::Button("OnUpdateDxLibHandle"))
        {
            if (mv1File_)
            {
                modelDxLibHandle_ = mv1File_->LoadDxLibHandle();
                RefreshTriangleListInfo();
            }
        }
        if (ImGui::Button("OnUpdateShaderConstantBuffer"))
        {
            if (cbHandle_ != -1)
            {
                DeleteShaderConstantBuffer(cbHandle_);
                cbHandle_ = -1;
            }
            GetOrCreateShaderConstantBufferHandle();
        }
        ImGui::Text("cbHandle_: %d  (slot b%d)", cbHandle_, CUSTOM_SHADER_CB_SLOT);
        ImGui::Text("triangleLists: %d  allRigid: %s",
                    static_cast<int>(rigidTriangleList_.size()), allRigid_ ? "true" : "false");
    }
}
