#include "ModelRenderer.h"
#include "../../../Core/Coroutine/Coroutine.h"
#include "../../GameObject/Transform/Transform.h"

void Component::ModelRenderer::InitRenderer()
{
    if (mv1File_)
    {
        modelDxLibHandle_ = mv1File_->DxLibHandle();
    }
}

void Component::ModelRenderer::OnShadowRender()
{
    if (!IsEnable())
        return;
    
    MV1SetMatrix(modelDxLibHandle_, Transform().GetDxWorldMatrix());
    MV1DrawModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnRender()
{
    if (!IsEnable())
        return;
    
    MV1SetMatrix(modelDxLibHandle_, Transform().GetDxWorldMatrix());
    MV1DrawModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnDestroy()
{
    MV1DeleteModel(modelDxLibHandle_);
}

void Component::ModelRenderer::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("mv1File_", mv1File_);
    if (ImGui::Button("OnUpdateDxLibHandle"))
    {
        if (mv1File_)
        {
            modelDxLibHandle_ = mv1File_->DxLibHandle();
        }
    }
}
