#include "Ui_ControlGuide.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Ui_ControlGuideRow.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Ui
{
    namespace
    {
        float ControlGuideMoveTowards(const float current, const float target, const float maxDelta)
        {
            if (current < target)
                return (std::min)(current + maxDelta, target);
            return (std::max)(current - maxDelta, target);
        }

        float ControlGuideStepRate(const float deltaTime, const float duration_secs)
        {
            return duration_secs > 0.0f ? deltaTime / duration_secs : 1.0f;
        }

        int ControlGuideToBlendRate(const float alpha)
        {
            return std::clamp(static_cast<int>(alpha), 0, 255);
        }
    }

    void ControlGuide::SpawnRows(const std::size_t count)
    {
        if (!rowViews_.empty() || !rowPrefab_)
            return;

        if (!rows_)
            return;
        const auto rowsObject = rows_.get();

        // 生成順を行の添字として使うので、生成に失敗した行も詰めずに残す
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab_.get(), rowsObject).lock();
            rowViews_.push_back(rowObject ? rowObject->Components().Catch<ControlGuideRow>() : std::weak_ptr<ControlGuideRow>{});
        }
        rowStates_.assign(count, RowState{});
    }

    void ControlGuide::Present(
        const bool isShown,
        const std::span<const RowRequest> requests,
        const std::optional<std::size_t> focusedRow,
        const bool isFocusCleared)
    {
        const float deltaTime = Time::DeltaTime();
        guideAlpha_ = ControlGuideMoveTowards(guideAlpha_, isShown ? 1.0f : 0.0f, ControlGuideStepRate(deltaTime, guideFadeDuration_secs_));
        focusElapsed_secs_ = focusedRow ? focusElapsed_secs_ + deltaTime : 0.0f;

        const std::size_t count = (std::min)({ rowStates_.size(), rowViews_.size(), requests.size() });
        anyFocusRate_ = 0.0f;
        for (std::size_t i = 0; i < count; ++i)
        {
            if (isShown)
                AnimateRow(rowStates_[i], requests[i], focusedRow == i, deltaTime);
            anyFocusRate_ = (std::max)(anyFocusRate_, rowStates_[i].focusRate);
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            if (const auto view = rowViews_[i].lock())
                PresentRow(*view, rowStates_[i], requests[i], isFocusCleared);
        }
    }

    std::optional<glm::vec2> ControlGuide::RowAnchor(const std::size_t row) const
    {
        if (row >= rowViews_.size() || guideAlpha_ <= 0.0f)
            return std::nullopt;

        const auto view = rowViews_[row].lock();
        const auto entity = view ? view->Entity().lock() : nullptr;
        if (!entity || !view->IsEnable())
            return std::nullopt;

        const glm::vec3 worldPos = entity->Transform().GetWorldPos();
        return glm::vec2(worldPos.x, worldPos.y);
    }

    void ControlGuide::AnimateRow(RowState& row, const RowRequest& request, const bool isFocused, const float deltaTime) const
    {
        const bool isActive = request.isShown && request.isUsable;
        const bool isLabelChanged = request.isShown && row.label != request.label;
        // 入力機器の切替で絵だけが変わった時は、光らせずに差し替える
        const bool isGlyphChanged = request.isShown && row.glyph != request.glyph;
        if (isActive && (!row.isActive || isLabelChanged))
            row.pulseElapsed_secs = 0.0f;
        else
            row.pulseElapsed_secs += deltaTime;
        row.isActive = isActive;

        // 消えていく行は直前の中身のままフェードさせる
        if (isLabelChanged || isGlyphChanged)
        {
            row.glyph          = request.glyph;
            row.label          = request.label;
            row.isContentDirty = true;
        }

        if (row.isFocused != isFocused)
        {
            row.isFocused    = isFocused;
            row.isFocusDirty = true;
        }

        const float step = ControlGuideStepRate(deltaTime, rowFadeDuration_secs_);
        row.visibility = ControlGuideMoveTowards(row.visibility, request.isShown  ? 1.0f : 0.0f, step);
        row.usableRate = ControlGuideMoveTowards(row.usableRate, request.isUsable ? 1.0f : 0.0f, step);
        row.focusRate  = ControlGuideMoveTowards(row.focusRate, isFocused ? 1.0f : 0.0f, ControlGuideStepRate(deltaTime, focusFadeDuration_secs_));
    }

    void ControlGuide::PresentRow(ControlGuideRow& view, RowState& row, const RowRequest& request, const bool isCleared) const
    {
        // 出始めた行はすぐ有効にして枠を確保し、消える行はフェードし終えてから無効にしてレイアウトから外す
        const bool isEnabled = guideAlpha_ > 0.0f && (request.isShown || row.visibility > 0.0f);
        if (view.IsEnable() != isEnabled)
        {
            if (const auto entity = view.Entity().lock())
                entity->SetEnable(isEnabled);
        }
        if (!isEnabled)
            return;

        if (row.isContentDirty)
        {
            view.SetContent(row.glyph, row.label);
            row.isContentDirty = false;
        }
        if (row.isFocusDirty)
        {
            view.SetFocused(row.isFocused);
            row.isFocusDirty = false;
        }

        const float usableAlphaRate = std::lerp(static_cast<float>(dimAlpha_) / 255.0f, 1.0f, row.usableRate);
        // 指されている行は常に最前面の明るさ、それ以外はフォーカス中だけさらに沈める
        const float focusDimRate = std::lerp(1.0f - unfocusedDimRate_ * anyFocusRate_, 1.0f, row.focusRate);
        const float bodyAlpha  = 255.0f * guideAlpha_ * row.visibility * (std::max)(usableAlphaRate, row.focusRate) * focusDimRate;
        const float pulse      = pulseDuration_secs_ > 0.0f ? (std::max)(0.0f, 1.0f - row.pulseElapsed_secs / pulseDuration_secs_) : 0.0f;
        const float pulseAlpha = pulse * guideAlpha_ * row.visibility;
        const float hidden     = 1.0f - row.visibility;
        // 行の枠もフェードと一緒に smoothstep で伸び縮みさせ、上下の行を跳ねさせない
        const float slotRate   = row.visibility * row.visibility * (3.0f - 2.0f * row.visibility);

        const float focusAlpha  = row.focusRate * guideAlpha_ * row.visibility;
        const float breath      = 0.5f + 0.5f * std::sin(focusElapsed_secs_ * 2.0f * std::numbers::pi_v<float> / (std::max)(focusPulsePeriod_secs_, 0.01f));
        const float markAlpha   = 255.0f * focusAlpha * std::lerp(0.55f, 1.0f, breath);

        view.Apply(ControlGuideRow::Appearance{
            .slotRate            = slotRate,
            .slideOffset_px      = -slideDistance_px_ * hidden * hidden,
            .bodyAlpha           = ControlGuideToBlendRate(bodyAlpha),
            .labelShadowAlpha    = ControlGuideToBlendRate(bodyAlpha * labelShadowAlphaRate_),
            .accentGlowAlpha     = ControlGuideToBlendRate(static_cast<float>(accentGlowMaxAlpha_) * pulseAlpha),
            .glyphFlashAlpha     = ControlGuideToBlendRate((std::max)(static_cast<float>(glyphFlashMaxAlpha_) * pulseAlpha,
                                                                     static_cast<float>(focusGlyphFlashMaxAlpha_) * focusAlpha * breath)),
            .focusStripAlpha     = ControlGuideToBlendRate(255.0f * focusAlpha),
            .focusArrowAlpha     = ControlGuideToBlendRate(isCleared ? 0.0f : markAlpha),
            .focusCheckAlpha     = ControlGuideToBlendRate(isCleared ? 255.0f * focusAlpha : 0.0f),
            .focusArrowOffset_px = focusArrowSwing_px_ * breath,
        });
    }

    void ControlGuide::OnDrawGui()
    {
        ImGui::Text("guideAlpha_: %.2f", guideAlpha_);

        ImGuiHelper::OnDrawInputField("rows_", rows_);
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("slideDistance_px_", slideDistance_px_);
        ImGuiHelper::OnDrawInputField("guideFadeDuration_secs_", guideFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("rowFadeDuration_secs_", rowFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("pulseDuration_secs_", pulseDuration_secs_);
        ImGuiHelper::OnDrawInputField("accentGlowMaxAlpha_", accentGlowMaxAlpha_);
        ImGuiHelper::OnDrawInputField("glyphFlashMaxAlpha_", glyphFlashMaxAlpha_);
        ImGuiHelper::OnDrawInputField("dimAlpha_", dimAlpha_);
        ImGuiHelper::OnDrawInputField("labelShadowAlphaRate_", labelShadowAlphaRate_);
        ImGuiHelper::OnDrawInputField("focusFadeDuration_secs_", focusFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("focusPulsePeriod_secs_", focusPulsePeriod_secs_);
        ImGuiHelper::OnDrawInputField("focusArrowSwing_px_", focusArrowSwing_px_);
        ImGuiHelper::OnDrawInputField("focusGlyphFlashMaxAlpha_", focusGlyphFlashMaxAlpha_);
        ImGuiHelper::OnDrawInputField("unfocusedDimRate_", unfocusedDimRate_);
    }
}
