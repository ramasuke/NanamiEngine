#include "Ui_BossHealthShardRenderer.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    namespace
    {
        class ScopedDrawState final
        {
        public:
            ScopedDrawState()
                : drawMode_(GetDrawMode())
            {
                GetDrawBlendMode(&blendMode_, &blendParam_);
            }
            ~ScopedDrawState()
            {
                SetDrawMode(drawMode_);
                SetDrawBlendMode(blendMode_, blendParam_);
            }
            ScopedDrawState(const ScopedDrawState&) = delete;
            ScopedDrawState& operator=(const ScopedDrawState&) = delete;

        private:
            int drawMode_   = DX_DRAWMODE_NEAREST;
            int blendMode_  = DX_BLENDMODE_NOBLEND;
            int blendParam_ = 0;
        };
    }

    void BossHealthShardRenderer::SetFill(const float fillRate, const float trailFillRate)
    {
        fillRate_      = std::clamp(fillRate, 0.0f, 1.0f);
        trailFillRate_ = std::clamp(trailFillRate, 0.0f, 1.0f);
    }

    void BossHealthShardRenderer::SetDanger(const bool isDanger)
    {
        isDanger_ = isDanger;
    }

    void BossHealthShardRenderer::DrawLayer(const int graphHandle, const float fillRate, const glm::vec2& pivot, const float angle) const
    {
        int width  = 0;
        int height = 0;
        GetGraphSize(graphHandle, &width, &height);

        // 根元から fillRate 分だけ切り出す。満タンの時は先端側の余白も含めて全体を描く
        const float baseY      = static_cast<float>(height) - padding_px_;
        const float fillLength = static_cast<float>(height) - padding_px_ * 2.0f;
        const int srcY = fillRate >= 1.0f ? 0
            : std::clamp(static_cast<int>(std::floor(baseY - fillRate * fillLength)), 0, height);
        const int drawHeight = height - srcY;
        if (drawHeight <= 0)
            return;

        DrawRectRotaGraph2F(
            pivot.x,
            pivot.y,
            0,
            srcY,
            width,
            drawHeight,
            static_cast<float>(width) * 0.5f,
            baseY + baseDistance_ - static_cast<float>(srcY),
            1.0,
            angle,
            graphHandle,
            TRUE);
    }

    void BossHealthShardRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !emptySprite_ || !fillSprite_)
            return;

        const ScopedDrawState drawState;
        SetDrawMode(DX_DRAWMODE_BILINEAR);

        const auto worldPos   = Transform().GetWorldPos();
        const glm::vec2 pivot = glm::vec2(worldPos.x, worldPos.y);
        const float angle     = glm::eulerAngles(Transform().GetWorldRot()).z;
        const auto fillSprite = (isDanger_ && fillDangerSprite_ ? fillDangerSprite_ : fillSprite_).get();

        DrawLayer(emptySprite_->GetDxLibHandle(), 1.0f, pivot, angle);
        if (trailSprite_ && trailFillRate_ > fillRate_)
            DrawLayer(trailSprite_->GetDxLibHandle(), trailFillRate_, pivot, angle);
        if (fillRate_ > 0.0f)
            DrawLayer(fillSprite->GetDxLibHandle(), fillRate_, pivot, angle);
    }

    void BossHealthShardRenderer::OnDrawGui()
    {
        ImGui::SliderFloat("fillRate (preview)", &fillRate_, 0.0f, 1.0f);
        ImGui::SliderFloat("trailFillRate (preview)", &trailFillRate_, 0.0f, 1.0f);
        ImGui::Checkbox("isDanger (preview)", &isDanger_);

        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("emptySprite_", emptySprite_);
        ImGuiHelper::OnDrawInputField("fillSprite_", fillSprite_);
        ImGuiHelper::OnDrawInputField("fillDangerSprite_", fillDangerSprite_);
        ImGuiHelper::OnDrawInputField("trailSprite_", trailSprite_);
        ImGuiHelper::OnDrawInputField("baseDistance_", baseDistance_);
        ImGuiHelper::OnDrawInputField("padding_px_", padding_px_);
    }
}
