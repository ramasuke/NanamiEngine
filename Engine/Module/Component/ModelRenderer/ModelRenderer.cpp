#include "ModelRenderer.h"
#include "../../../Core/Coroutine/Coroutine.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include <../../Libs/glm/gtc/matrix_transform.hpp>

void Component::ModelRenderer::InitRenderer()
{
    if (mv1File_)
    {
        modelDxLibHandle_ = mv1File_->LoadDxLibHandle();
    }
}

void Component::ModelRenderer::OnPreFixedUpdate()
{
    if (!useFixedInterpolation_)
        return;
    prevWorldPos_   = Transform().GetWorldPos();
    prevWorldRot_   = Transform().GetWorldRot();
    hasPrevCapture_ = true;
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

MATRIX Component::ModelRenderer::GetRenderMatrix() const
{
    if (useFixedInterpolation_ && hasPrevCapture_)
    {
        const float alpha     = Time::GetFixedAlpha();
        const glm::vec3 pos   = glm::mix  (prevWorldPos_,   Transform().GetWorldPos(),   alpha);
        const glm::quat rot   = glm::slerp(prevWorldRot_,   Transform().GetWorldRot(),   alpha);
        const glm::vec3 scale = Transform().GetWorldScale();
        const glm::mat4 mat   = glm::translate(glm::mat4(1.0f), pos)
                              * glm::mat4_cast(rot)
                              * glm::scale(glm::mat4(1.0f), scale);
        return GlmMatToDxMat(mat);
    }
    return Transform().GetDxWorldMatrix();
}

void Component::ModelRenderer::OnShadowRender()
{
    if (!IsEnable())
        return;

    MV1SetMatrix(modelDxLibHandle_, GetRenderMatrix());
    MV1DrawModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnRender()
{
    if (!IsEnable())
        return;

    MV1SetMatrix(modelDxLibHandle_, GetRenderMatrix());
    MV1DrawModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnDestroy()
{
    MV1DeleteModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("mv1File_", mv1File_);
    ImGuiHelper::OnDrawInputField("useFixedInterpolation_", useFixedInterpolation_);
    if (ImGui::Button("OnUpdateDxLibHandle"))
    {
        if (mv1File_)
        {
            modelDxLibHandle_ = mv1File_->LoadDxLibHandle();
        }
    }
}
