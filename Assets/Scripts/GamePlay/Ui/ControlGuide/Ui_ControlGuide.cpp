#include "Ui_ControlGuide.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Ui_ControlGuideRow.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        tweeny::tween<float> ControlGuideFadeTween(const float duration_secs)
        {
            return tweeny::from(0.0f).to(1.0f).during(LibCore::Tween::Ms(duration_secs));
        }

        void ControlGuideFade(LibCore::Tween::TweenPlayer<float>& fade, const bool isOn, const float deltaTime)
        {
            if (isOn)
                fade.PlayForward();
            else
                fade.PlayBackward();
            fade.Tick(deltaTime);
        }

        void ControlGuidePlayPulse(LibCore::Tween::TweenPlayer<float>& pulse, const float duration_secs)
        {
            if (duration_secs > 0.0f)
                pulse.Play(tweeny::from(1.0f).to(0.0f).during(LibCore::Tween::Ms(duration_secs)));
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
        for (auto& row : rowStates_)
        {
            row.visibility.Set(ControlGuideFadeTween(rowFadeDuration_secs_));
            row.usableRate.Set(ControlGuideFadeTween(rowFadeDuration_secs_));
            row.focusRate .Set(ControlGuideFadeTween(focusFadeDuration_secs_));
            ControlGuidePlayPulse(row.pulse, pulseDuration_secs_);
        }
        guideFade_.Set(ControlGuideFadeTween(guideFadeDuration_secs_));
    }

    void ControlGuide::Present(
        const bool isShown,
        const std::span<const RowRequest> requests,
        const std::optional<std::size_t> focusedRow,
        const bool isFocusCleared)
    {
        const float deltaTime = Time::DeltaTime();
        ControlGuideFade(guideFade_, isShown, deltaTime);
        focusElapsed_secs_ = focusedRow ? focusElapsed_secs_ + deltaTime : 0.0f;

        const std::size_t count = (std::min)({ rowStates_.size(), rowViews_.size(), requests.size() });
        anyFocusRate_ = 0.0f;
        for (std::size_t i = 0; i < count; ++i)
        {
            if (isShown)
                AnimateRow(rowStates_[i], requests[i], focusedRow == i, deltaTime);
            anyFocusRate_ = (std::max)(anyFocusRate_, rowStates_[i].focusRate.Value());
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            if (const auto view = rowViews_[i].lock())
                PresentRow(*view, rowStates_[i], requests[i], isFocusCleared);
        }
    }

    std::optional<glm::vec2> ControlGuide::RowAnchor(const std::size_t row) const
    {
        if (row >= rowViews_.size() || guideFade_.Value() <= 0.0f)
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
            ControlGuidePlayPulse(row.pulse, pulseDuration_secs_);
        else
            row.pulse.Tick(deltaTime);
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

        ControlGuideFade(row.visibility, request.isShown,  deltaTime);
        ControlGuideFade(row.usableRate, request.isUsable, deltaTime);
        ControlGuideFade(row.focusRate,  isFocused,        deltaTime);
    }

    void ControlGuide::PresentRow(ControlGuideRow& view, RowState& row, const RowRequest& request, const bool isCleared) const
    {
        // 出始めた行はすぐ有効にして枠を確保し、消える行はフェードし終えてから無効にしてレイアウトから外す
        const float guideAlpha = guideFade_.Value();
        const float visibility = row.visibility.Value();
        const float focusRate  = row.focusRate.Value();
        const bool isEnabled = guideAlpha > 0.0f && (request.isShown || visibility > 0.0f);
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

        const float usableAlphaRate = std::lerp(static_cast<float>(dimAlpha_) / 255.0f, 1.0f, row.usableRate.Value());
        // 指されている行は常に最前面の明るさ、それ以外はフォーカス中だけさらに沈める
        const float focusDimRate = std::lerp(1.0f - unfocusedDimRate_ * anyFocusRate_, 1.0f, focusRate);
        const float bodyAlpha  = 255.0f * guideAlpha * visibility * (std::max)(usableAlphaRate, focusRate) * focusDimRate;
        const float pulseAlpha = row.pulse.Value() * guideAlpha * visibility;
        const float hidden     = 1.0f - visibility;
        // 行の枠もフェードと一緒に smoothstep で伸び縮みさせ、上下の行を跳ねさせない
        const float slotRate   = visibility * visibility * (3.0f - 2.0f * visibility);

        const float focusAlpha  = focusRate * guideAlpha * visibility;
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
        ImGui::Text("guideAlpha_: %.2f", guideFade_.Value());

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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ControlGuide);
#pragma endregion
