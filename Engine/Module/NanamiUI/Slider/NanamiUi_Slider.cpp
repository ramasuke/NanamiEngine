#include "NanamiUi_Slider.h"
#include <algorithm>
#include <cmath>
#include <numbers>

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../../Core/Application/Time/Time.h"

namespace
{
    // 描画範囲・ブレンドモードを抜けるときに元へ戻す
    class ScopedDrawState final
    {
    public:
        ScopedDrawState()
        {
            GetDrawBlendMode(&blendMode_, &blendParam_);
        }
        ~ScopedDrawState()
        {
            int screenW = 0;
            int screenH = 0;
            GetDrawScreenSize(&screenW, &screenH);
            SetDrawArea(0, 0, screenW, screenH);
            SetDrawBlendMode(blendMode_, blendParam_);
        }
        ScopedDrawState(const ScopedDrawState&) = delete;
        ScopedDrawState& operator=(const ScopedDrawState&) = delete;

    private:
        int blendMode_  = DX_BLENDMODE_NOBLEND;
        int blendParam_ = 0;
    };

    float EaseInOutCubic(const float t)
    {
        if (t < 0.5f)
            return 4.0f * t * t * t;
        const float u = -2.0f * t + 2.0f;
        return 1.0f - u * u * u * 0.5f;
    }
}

namespace NanamiEngine::Module::NanamiUi
{
    void Slider::SetValue(const float value)
    {
        const float next = std::clamp(value, 0.0f, 1.0f);
        trailValue_ = std::max(trailValue_, next);
        if (next < value_)
        {
            trailFrom_           = trailValue_;
            trailWaitTimer_secs_ = trailDelay_secs_;
            trailElapsed_secs_   = 0.0f;
        }
        value_ = next;
    }

    void Slider::ChangeGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite)
    {
        if (!sprite || gaugeSprite_.get() == sprite)
            return;

        fadingOutGaugeSprite_ = gaugeSprite_.get();
        gaugeSprite_          = sprite;
        gaugeFadeTimer_secs_  = gaugeFadeDuration_secs_;
    }

    void Slider::SetPulse(const bool isPulsing)
    {
        if (isPulsing && !isPulsing_)
            pulseTime_secs_ = 0.0f;
        isPulsing_ = isPulsing;
    }

    void Slider::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        if (trailWaitTimer_secs_ > 0.0f)
        {
            trailWaitTimer_secs_ = std::max(0.0f, trailWaitTimer_secs_ - deltaTime);
        }
        else if (trailValue_ > value_)
        {
            trailElapsed_secs_ += deltaTime;
            const float t = trailDuration_secs_ > 0.0f
                ? std::clamp(trailElapsed_secs_ / trailDuration_secs_, 0.0f, 1.0f)
                : 1.0f;
            // 回復で value_ が上がってもトレイルが逆戻りしないよう、現在位置より上には戻さない
            trailValue_ = std::clamp(std::lerp(trailFrom_, value_, EaseInOutCubic(t)), value_, trailValue_);
        }

        if (gaugeFadeTimer_secs_ > 0.0f)
        {
            gaugeFadeTimer_secs_ = std::max(0.0f, gaugeFadeTimer_secs_ - deltaTime);
            if (gaugeFadeTimer_secs_ <= 0.0f)
                fadingOutGaugeSprite_.reset();
        }

        if (isPulsing_)
            pulseTime_secs_ += deltaTime;
    }

    Slider::DrawRect Slider::CalcDrawRect() const
    {
        const auto worldPos = Transform().GetWorldPos();
        return DrawRect{
            static_cast<int>(worldPos.x),
            static_cast<int>(worldPos.y),
            static_cast<int>(drawSize_.x),
            static_cast<int>(drawSize_.y) };
    }

    void Slider::DrawLayer(const DrawRect& rect, const int graphHandle, const float fillRate) const
    {
        const int clipWidth = static_cast<int>(static_cast<float>(rect.w) * std::clamp(fillRate, 0.0f, 1.0f));
        if (clipWidth <= 0)
            return;

        SetDrawArea(rect.x, rect.y, rect.x + clipWidth, rect.y + rect.h);
        if (isStretchToDrawSize_)
        {
            DrawExtendGraphF(
                static_cast<float>(rect.x),
                static_cast<float>(rect.y),
                static_cast<float>(rect.x + rect.w),
                static_cast<float>(rect.y + rect.h),
                graphHandle,
                TRUE);
        }
        else
        {
            DrawRotaGraphF(
                static_cast<float>(rect.x) + drawPosition_.x,
                static_cast<float>(rect.y) + drawPosition_.y,
                Transform().GetWorldScale().x,
                Transform().GetWorldRot().z,
                graphHandle,
                TRUE);
        }
    }

    void Slider::DrawTicks(const DrawRect& rect) const
    {
        if (tickCount_ <= 1)
            return;

        const int top    = static_cast<int>(std::round(static_cast<float>(rect.y) + bandInsetY_ + tickInsetY_));
        const int bottom = static_cast<int>(std::round(static_cast<float>(rect.y + rect.h) - bandInsetY_ - tickInsetY_));
        if (bottom <= top)
            return;

        SetDrawArea(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        for (int i = 1; i < tickCount_; ++i)
        {
            const int x = rect.x + static_cast<int>(std::round(static_cast<float>(rect.w * i) / static_cast<float>(tickCount_)));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickShadowAlpha_, 0, 255));
            DrawBox(x, top, x + 1, bottom, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickHighlightAlpha_, 0, 255));
            DrawBox(x + 1, top, x + 2, bottom, GetColor(255, 255, 255), TRUE);
        }
    }

    void Slider::DrawTip(const DrawRect& rect) const
    {
        if (!tipSprite_ || value_ <= 0.0f || value_ >= 1.0f)
            return;

        const float top    = static_cast<float>(rect.y) + bandInsetY_;
        const float bottom = static_cast<float>(rect.y + rect.h) - bandInsetY_;
        const float endX   = static_cast<float>(rect.x) + static_cast<float>(rect.w) * value_;

        SetDrawArea(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        DrawExtendGraphF(endX - tipWidth_, top, endX, bottom, tipSprite_->GetDxLibHandle(), TRUE);
    }

    void Slider::OnUserInterfaceRender()
    {
        if (!IsEnable() || !gaugeSprite_)
            return;

        const DrawRect rect = CalcDrawRect();
        {
            const ScopedDrawState drawState;
            if (backgroundSprite_)
                DrawLayer(rect, backgroundSprite_->GetDxLibHandle(), 1.0f);
            if (trailSprite_)
                DrawLayer(rect, trailSprite_->GetDxLibHandle(), trailValue_);
        }

        if (!isStretchToDrawSize_)
            DrawMaskedGauge(rect);

        {
            const ScopedDrawState drawState;
            if (isStretchToDrawSize_)
            {
                const int gaugeHandle = gaugeSprite_->GetDxLibHandle();
                if (fadingOutGaugeSprite_ && gaugeFadeDuration_secs_ > 0.0f)
                {
                    const float fadeInRate = 1.0f - gaugeFadeTimer_secs_ / gaugeFadeDuration_secs_;
                    DrawLayer(rect, fadingOutGaugeSprite_->GetDxLibHandle(), value_);
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255.0f * std::clamp(fadeInRate, 0.0f, 1.0f)));
                    DrawLayer(rect, gaugeHandle, value_);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else
                {
                    DrawLayer(rect, gaugeHandle, value_);
                }

                if (isPulsing_ && pulseMaxAlpha_ > 0)
                {
                    const float wave = 0.5f + 0.5f * std::sin(pulseTime_secs_ * pulseFrequency_hz_ * 2.0f * std::numbers::pi_v<float>);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(pulseMaxAlpha_) * wave));
                    DrawLayer(rect, gaugeHandle, value_);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
            }

            DrawTicks(rect);
            DrawTip(rect);
        }
    }

    void Slider::DrawMaskedGauge(const DrawRect& rect) const
    {
        const int w = rect.w;
        const int h = rect.h;
        const int maskedScreen = MakeScreen(w, h, true);

        //マスク生成フェーズ
        SetDrawScreen(maskedScreen);
        ClsDrawScreen();

        const auto renderRot   = Transform().GetWorldRot  ();
        const auto renderScale = Transform().GetWorldScale();

        const float angle = renderRot  .z;
        const float scale = renderScale.x;

        // マスク内ローカル座標で描画
        DrawRotaGraphF(
            drawPosition_.x,
            drawPosition_.y,
            scale,
            angle,
            gaugeSprite_->GetDxLibHandle(),
            TRUE
        );

        // 減少分を黒で塗る
        const int lostWidth = static_cast<int>(w * (1.0f - value_));
        DrawBox(w - lostWidth, 0, w, h, GetColor(0, 0, 0), TRUE);

        // 黒を透過
        GraphFilter(maskedScreen, DX_GRAPH_FILTER_BRIGHT_CLIP, DX_CMP_LESS, 20, TRUE, GetColor(0, 255, 0), 0);
        SetDrawScreen(DX_SCREEN_BACK);

        //最終描画フェーズ
        GraphFilter(maskedScreen, DX_GRAPH_FILTER_BRIGHT_CLIP, DX_CMP_GREATER, 128, TRUE, GetColor(0, 255, 0), 0);
        DrawGraph(rect.x, rect.y, maskedScreen, TRUE);
        DeleteGraph(maskedScreen);

    }

    void Slider::OnDrawGui()
    {
        ImGui::Text("Slider");

        if (ImGui::SliderFloat("value_", &value_, 0.0f, 1.0f))
        {
            trailValue_ = value_;
            trailFrom_  = value_;
        }

        float position[2] = { drawPosition_.x,  drawPosition_.y };
        float size    [2] = { drawSize_    .x, drawSize_     .y };

        if (ImGui::InputFloat2("drawPos_", position))
        {
            drawPosition_.x = position[0];
            drawPosition_.y = position[1];
        }

        if (ImGui::InputFloat2("drawSize_", size))
        {
            drawSize_.x = size[0];
            drawSize_.y = size[1];
        }

        ImGuiHelper::OnDrawInputField("gaugeSprite_", gaugeSprite_);
        value_ = std::clamp(value_, 0.0f, 1.0f);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("isStretchToDrawSize_", isStretchToDrawSize_);
        ImGuiHelper::OnDrawInputField("backgroundSprite_", backgroundSprite_);
        ImGuiHelper::OnDrawInputField("trailSprite_", trailSprite_);
        ImGuiHelper::OnDrawInputField("trailDelay_secs_", trailDelay_secs_);
        ImGuiHelper::OnDrawInputField("trailDuration_secs_", trailDuration_secs_);
        ImGuiHelper::OnDrawInputField("tipSprite_", tipSprite_);
        ImGuiHelper::OnDrawInputField("tipWidth_", tipWidth_);
        ImGuiHelper::OnDrawInputField("tickCount_", tickCount_);
        ImGuiHelper::OnDrawInputField("tickInsetY_", tickInsetY_);
        ImGuiHelper::OnDrawInputField("tickShadowAlpha_", tickShadowAlpha_);
        ImGuiHelper::OnDrawInputField("tickHighlightAlpha_", tickHighlightAlpha_);
        ImGuiHelper::OnDrawInputField("bandInsetY_", bandInsetY_);
        ImGuiHelper::OnDrawInputField("gaugeFadeDuration_secs_", gaugeFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("pulseFrequency_hz_", pulseFrequency_hz_);
        ImGuiHelper::OnDrawInputField("pulseMaxAlpha_", pulseMaxAlpha_);
        ImGui::Text("trailValue_: %.3f  isPulsing_: %d", trailValue_, isPulsing_ ? 1 : 0);

        const int w = static_cast<int>(drawSize_.x);
        const int h = static_cast<int>(drawSize_.y);
        const auto worldPos = Transform().GetWorldPos();
        const int x = static_cast<int>(worldPos.x);
        const int y = static_cast<int>(worldPos.y);
        DrawBox(x, y, x + w, y + h, GetColor(255, 255, 255), FALSE);
    }
}
