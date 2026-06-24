#include "ModelRenderer.h"
#include "../../../Core/Coroutine/Coroutine.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include <../../Libs/glm/gtc/matrix_transform.hpp>

namespace NanamiEngine::Module::Component
{
    void ModelRenderer::InitRenderer()
    {
        if (mv1File_)
            modelDxLibHandle_ = mv1File_->LoadDxLibHandle();

        if (HasCustomShader())
            cbHandle_ = CreateShaderConstantBuffer(256);
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

    void ModelRenderer::ApplyCustomShader()
    {
        SetUseLighting(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        SetWriteZBuffer3D(FALSE);
        SetUseVertexShader(vsFile_->GetVsHandle());
        SetUsePixelShader(psFile_->GetPsHandle());
        if (cbHandle_ != -1)
            SetShaderConstantBuffer(cbHandle_, DX_SHADERTYPE_PIXEL, 1);
        MV1SetUseOrigShader(TRUE);
    }

    void ModelRenderer::RestoreCustomShader()
    {
        MV1SetUseOrigShader(FALSE);
        SetUseVertexShader(-1);
        SetUsePixelShader(-1);
        SetUseLighting(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void ModelRenderer::OnRender()
    {
        if (!IsEnable() || modelDxLibHandle_ == -1)
            return;

        MV1SetMatrix(modelDxLibHandle_, GetRenderMatrix());

        if (HasCustomShader())
        {
            ApplyCustomShader();
            MV1DrawModel(modelDxLibHandle_);
            RestoreCustomShader();
        }
        else
        {
            MV1DrawModel(modelDxLibHandle_);
        }
    }

    void ModelRenderer::OnDestroy()
    {
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
                modelDxLibHandle_ = mv1File_->LoadDxLibHandle();
        }
        if (ImGui::Button("OnUpdateShaderConstantBuffer"))
        {
            if (cbHandle_ != -1)
                DeleteShaderConstantBuffer(cbHandle_);
            cbHandle_ = HasCustomShader() ? CreateShaderConstantBuffer(256) : -1;
        }
    }
}
