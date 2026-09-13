#include "Ui_BossHealthGauge.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "DxLib.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
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

    void BossHealthGauge::Show(const std::string& bossName)
    {
        if (!bossNameText_ && !bossNameTextName_.empty())
            bossNameText_ = GameObject::CatchChild<NanamiUi::TextRenderer>(Entity(), bossNameTextName_);
        if (bossNameText_)
            bossNameText_->SetText(bossName);

        value_               = 0.0f;
        trailValue_          = 0.0f;
        trailWaitTimer_secs_ = 0.0f;
        introElapsed_secs_   = 0.0f;
        pulseTime_secs_      = 0.0f;
        Entity().lock()->SetEnable(true);
    }

    void BossHealthGauge::SetHealthRate(const float healthRate)
    {
        targetRate_ = std::clamp(healthRate, 0.0f, 1.0f);
        if (!IsIntroPlaying())
            ApplyValue(targetRate_);
    }

    void BossHealthGauge::ApplyValue(const float healthRate)
    {
        if (healthRate < value_)
        {
            trailValue_          = std::max(trailValue_, value_);
            trailWaitTimer_secs_ = trailDelay_secs_;
        }
        else
        {
            trailValue_ = healthRate;
        }
        value_ = healthRate;
    }

    bool BossHealthGauge::IsIntroPlaying() const
    {
        return introElapsed_secs_ < introFillDuration_secs_;
    }

    bool BossHealthGauge::IsDanger() const
    {
        return value_ > 0.0f && value_ <= dangerHealthRate_;
    }

    void BossHealthGauge::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        if (IsIntroPlaying())
        {
            introElapsed_secs_ += deltaTime;
            const float introRate = introFillDuration_secs_ > 0.0f ? std::min(1.0f, introElapsed_secs_ / introFillDuration_secs_) : 1.0f;
            value_      = std::min(targetRate_, introRate);
            trailValue_ = value_;
        }

        if (trailWaitTimer_secs_ > 0.0f)
            trailWaitTimer_secs_ = std::max(0.0f, trailWaitTimer_secs_ - deltaTime);
        else
            trailValue_ = std::max(value_, trailValue_ - trailSpeed_perSec_ * deltaTime);

        pulseTime_secs_ = IsDanger() ? pulseTime_secs_ + deltaTime : 0.0f;
    }

    void BossHealthGauge::DrawShard(const int graphHandle, const float fillRate, const glm::vec2& pivot, const float angle) const
    {
        int width  = 0;
        int height = 0;
        GetGraphSize(graphHandle, &width, &height);

        // 根元（画像の下側）から fillRate 分だけ切り出す。満タンの時は先端側の余白（グロー）も含めて全体を描く
        const float baseY = static_cast<float>(height) - shardPadding_px_;
        const float fillLength = static_cast<float>(height) - shardPadding_px_ * 2.0f;
        const int srcY = fillRate >= 1.0f
            ? 0
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
            baseY + shardBaseDistance_ - static_cast<float>(srcY),
            1.0,
            angle,
            graphHandle,
            TRUE);
    }

    void BossHealthGauge::OnUserInterfaceRender()
    {
        if (!IsEnable() || !crestSprite_ || !shardEmptySprite_ || !shardFillSprite_)
            return;

        const ScopedDrawState drawState;
        SetDrawMode(DX_DRAWMODE_BILINEAR);

        const auto worldPos    = Transform().GetWorldPos();
        const glm::vec2 pivot  = glm::vec2(worldPos.x, worldPos.y) + pivotOffset_;
        const bool isDanger    = IsDanger();
        const auto fillSprite  = (isDanger && shardFillDangerSprite_ ? shardFillDangerSprite_ : shardFillSprite_).get();
        const float stepDeg    = shardCount_ > 1 ? arcAngle_deg_ / static_cast<float>(shardCount_ - 1) : 0.0f;
        const float startDeg   = shardCount_ > 1 ? -arcAngle_deg_ * 0.5f : 0.0f;
        const float count     = static_cast<float>(shardCount_);

        for (int i = 0; i < shardCount_; ++i)
        {
            const float angle     = (startDeg + stepDeg * static_cast<float>(i)) * std::numbers::pi_v<float> / 180.0f;
            const float fill      = std::clamp(value_      * count - static_cast<float>(i), 0.0f, 1.0f);
            const float trailFill = std::clamp(trailValue_ * count - static_cast<float>(i), 0.0f, 1.0f);

            DrawShard(shardEmptySprite_->GetDxLibHandle(), 1.0f, pivot, angle);
            if (shardTrailSprite_ && trailFill > fill)
                DrawShard(shardTrailSprite_->GetDxLibHandle(), trailFill, pivot, angle);
            if (fill > 0.0f)
                DrawShard(fillSprite->GetDxLibHandle(), fill, pivot, angle);
        }

        const glm::vec2 crestPos = pivot + crestOffset_;
        DrawRotaGraphF(crestPos.x, crestPos.y, 1.0, 0.0, crestSprite_->GetDxLibHandle(), TRUE);

        if (isDanger && crestGlowSprite_ && pulseMaxAlpha_ > 0)
        {
            const float wave = 0.5f + 0.5f * std::sin(pulseTime_secs_ * pulseFrequency_hz_ * 2.0f * std::numbers::pi_v<float>);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(std::clamp(pulseMaxAlpha_, 0, 255)) * wave));
            DrawRotaGraphF(crestPos.x, crestPos.y, 1.0, 0.0, crestGlowSprite_->GetDxLibHandle(), TRUE);
        }
    }

    void BossHealthGauge::OnDrawGui()
    {
        float previewRate = targetRate_;
        if (ImGui::SliderFloat("healthRate (preview)", &previewRate, 0.0f, 1.0f))
            SetHealthRate(previewRate);
        ImGui::Text("value_: %.3f  trailValue_: %.3f", value_, trailValue_);

        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("bossNameTextName_", bossNameTextName_);
        ImGuiHelper::OnDrawInputField("crestSprite_", crestSprite_);
        ImGuiHelper::OnDrawInputField("crestGlowSprite_", crestGlowSprite_);
        ImGuiHelper::OnDrawInputField("shardEmptySprite_", shardEmptySprite_);
        ImGuiHelper::OnDrawInputField("shardFillSprite_", shardFillSprite_);
        ImGuiHelper::OnDrawInputField("shardFillDangerSprite_", shardFillDangerSprite_);
        ImGuiHelper::OnDrawInputField("shardTrailSprite_", shardTrailSprite_);
        ImGuiHelper::OnDrawInputField("shardCount_", shardCount_);
        ImGuiHelper::OnDrawInputField("arcAngle_deg_", arcAngle_deg_);
        ImGuiHelper::OnDrawInputField("shardBaseDistance_", shardBaseDistance_);
        ImGuiHelper::OnDrawInputField("shardPadding_px_", shardPadding_px_);
        ImGui::InputFloat2("pivotOffset_", &pivotOffset_.x);
        ImGui::InputFloat2("crestOffset_", &crestOffset_.x);
        ImGuiHelper::OnDrawInputField("dangerHealthRate_", dangerHealthRate_);
        ImGuiHelper::OnDrawInputField("trailDelay_secs_", trailDelay_secs_);
        ImGuiHelper::OnDrawInputField("trailSpeed_perSec_", trailSpeed_perSec_);
        ImGuiHelper::OnDrawInputField("pulseFrequency_hz_", pulseFrequency_hz_);
        ImGuiHelper::OnDrawInputField("pulseMaxAlpha_", pulseMaxAlpha_);
        ImGuiHelper::OnDrawInputField("introFillDuration_secs_", introFillDuration_secs_);
        shardCount_ = std::max(shardCount_, 1);
    }
}
