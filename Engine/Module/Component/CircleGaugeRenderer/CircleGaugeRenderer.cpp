#include "CircleGaugeRenderer.h"

#include <algorithm>

#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void CircleGaugeRenderer::InitRenderer()
    {
    }

    void CircleGaugeRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !spriteFile_ || fillRate_ <= 0.0f || blendRate_ <= 0)
            return;

        const auto renderPos   = Transform().GetWorldPos  ();
        const auto renderScale = Transform().GetWorldScale();
        const int  handle      = spriteFile_->GetDxLibHandle();

        const double start = startPercent_;
        const double end   = start + static_cast<double>(spanPercent_) * std::clamp(fillRate_, 0.0f, 1.0f);

        SetDrawBlendMode(static_cast<int>(blendMode_), blendRate_);
        if (end <= 100.0)
        {
            DrawCircleGaugeF(renderPos.x, renderPos.y, end, handle, start, renderScale.x, FALSE, FALSE);
        }
        else
        {
            DrawCircleGaugeF(renderPos.x, renderPos.y, 100.0, handle, start, renderScale.x, FALSE, FALSE);
            DrawCircleGaugeF(renderPos.x, renderPos.y, end - 100.0, handle, 0.0, renderScale.x, FALSE, FALSE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void CircleGaugeRenderer::SetBlendRate(const int blendRate)
    {
        blendRate_ = blendRate;
    }

    void CircleGaugeRenderer::SetSprite(const std::weak_ptr<Asset::SpriteFile>& sprite)
    {
        spriteFile_ = sprite;
    }

    void CircleGaugeRenderer::SetFillRate(const float fillRate)
    {
        fillRate_ = fillRate;
    }

    void CircleGaugeRenderer::OnDrawGui()
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
        ImGuiHelper::OnDrawInputField("startPercent_", startPercent_);
        ImGuiHelper::OnDrawInputField("spanPercent_", spanPercent_);
        ImGui::SliderFloat("fillRate_", &fillRate_, 0.0f, 1.0f);
    }
}
