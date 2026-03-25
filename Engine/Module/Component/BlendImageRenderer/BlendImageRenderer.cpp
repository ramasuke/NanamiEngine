#include "BlendImageRenderer.h"

#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void BlendImageRenderer::InitRenderer()
    {
        
    }

    void BlendImageRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !spriteFile_)
            return;
        
        const auto renderPos    = Transform().GetWorldPos  ();
        const auto renderRot    = Transform().GetWorldRot  ();
        const auto renderScale  = Transform().GetWorldScale();

        const float angle = renderRot  .z;
        const float scale = renderScale.x;
        
        SetDrawBlendMode(static_cast<int>(blendMode_), blendRate_);
        DrawRotaGraphF(
            renderPos.x,
            renderPos.y,     
            scale,                        
            angle,                        
            spriteFile_->GetDxLibHandle(),
            TRUE                          
        );
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void BlendImageRenderer::SetBlendRate(const int blendRate)
    {
        blendRate_ = blendRate;
    }

    void BlendImageRenderer::OnDrawGui()
    {
        int mode = static_cast<int>(blendMode_);
        if (ImGui::Combo("BlendMode", &mode, Dxlib::BlendModeLabelNames, IM_ARRAYSIZE(Dxlib::BlendModeLabelNames)))
        {
            blendMode_ = static_cast<Dxlib::BlendMode>(mode);
        }
        ImGui::InputInt("BlendRate", &blendRate_);
        blendRate_ = std::clamp(blendRate_, 0, 255);
        ImGuiHelper::OnDrawInputField("spriteFile_", spriteFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
    }
}
