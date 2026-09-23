#include "Ui_GaugeEffects.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "DxLib.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    // NOTE: HP の割合は health / maxHealth で出るので、満タンは誤差込みで見る
    constexpr float fullValueThreshold = 1.0f - 1.0e-4f;

    // 描画範囲・ブレンドモード・描画モード・描画輝度を抜けるときに元へ戻す
    class ScopedDrawState final
    {
    public:
        explicit ScopedDrawState(const bool isBilinear)
            : drawMode_(GetDrawMode())
        {
            GetDrawBlendMode(&blendMode_, &blendParam_);
            GetDrawBright(&brightR_, &brightG_, &brightB_);
            if (isBilinear)
                SetDrawMode(DX_DRAWMODE_BILINEAR);
        }
        ~ScopedDrawState()
        {
            int screenW = 0;
            int screenH = 0;
            GetDrawScreenSize(&screenW, &screenH);
            SetDrawArea(0, 0, screenW, screenH);
            SetDrawMode(drawMode_);
            SetDrawBlendMode(blendMode_, blendParam_);
            SetDrawBright(brightR_, brightG_, brightB_);
        }
        ScopedDrawState(const ScopedDrawState&) = delete;
        ScopedDrawState& operator=(const ScopedDrawState&) = delete;

    private:
        int drawMode_   = DX_DRAWMODE_NEAREST;
        int blendMode_  = DX_BLENDMODE_NOBLEND;
        int blendParam_ = 0;
        int brightR_    = 255;
        int brightG_    = 255;
        int brightB_    = 255;
    };
}

namespace GamePlay::Ui
{
    void GaugeEffects::SnapTrail()
    {
        const auto slider = FindSlider();
        if (!slider)
            return;

        hasObservedValue_ = true;
        lastValue_  = slider->GetValue();
        trailValue_ = lastValue_;
        trailFrom_  = lastValue_;
        trailTween_.Stop();
        healTween_.Stop();
        fullTween_.Stop();
    }

    void GaugeEffects::ChangeGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite)
    {
        const auto slider = FindSlider();
        if (!slider || !sprite || slider->GetGaugeSprite() == sprite)
            return;

        fadingOutGaugeSprite_ = slider->GetGaugeSprite();
        slider->SetGaugeSprite(sprite);
        gaugeFadeTween_.Play(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(gaugeFadeDuration_secs_))
            .via(LibCore::Tween::Ease(LibCore::EaseType::Linear)));
    }

    void GaugeEffects::SetPulse(const bool isPulsing)
    {
        if (isPulsing && !isPulsing_)
            pulseTime_secs_ = 0.0f;
        isPulsing_ = isPulsing;
    }

    std::shared_ptr<NanamiUi::Slider> GaugeEffects::FindSlider()
    {
        if (auto slider = slider_.lock())
            return slider;

        const auto entity = Entity().lock();
        if (!entity)
            return nullptr;

        slider_ = entity->Components().Catch<NanamiUi::Slider>();
        return slider_.lock();
    }

    void GaugeEffects::UpdateTrail(const float value, const float deltaTime)
    {
        if (!hasObservedValue_)
        {
            hasObservedValue_ = true;
            lastValue_  = value;
            trailValue_ = value;
            trailFrom_  = value;
        }

        if (value != lastValue_)
        {
            trailValue_ = std::max(trailValue_, value);
            if (value > lastValue_)
            {
                StartHealTrail(lastValue_);
                if (value >= fullValueThreshold && lastValue_ < fullValueThreshold)
                    StartFullEffect();
            }
            else
            {
                trailFrom_ = trailValue_;
                trailTween_.Play(tweeny::from(0.0f)
                    .to(0.0f).during(LibCore::Tween::Ms(trailDelay_secs_))
                    .to(1.0f).during(LibCore::Tween::Ms(trailDuration_secs_))
                    .via(LibCore::Tween::Ease(LibCore::EaseType::InOutCubic)));
            }
            lastValue_ = value;
        }

        if (trailTween_.IsPlaying())
        {
            trailTween_.Tick(deltaTime);
            // 回復で値が上がってもトレイルが逆戻りしないよう、現在位置より上には戻さない
            trailValue_ = std::clamp(std::lerp(trailFrom_, value, trailTween_.Value()), value, trailValue_);
        }
    }

    void GaugeEffects::StartHealTrail(const float fromValue)
    {
        // 光が消えきる前に続けて回復したら、まだ光っている下端から伸ばす
        const bool isGlowing = healTween_.IsPlaying() && healFrom_ < fromValue;
        healFrom_ = isGlowing ? std::lerp(healFrom_, fromValue, healTween_.Value()) : fromValue;
        healTween_.Play(tweeny::from(0.0f)
            .to(0.0f).during(LibCore::Tween::Ms(healTrailHold_secs_))
            .to(1.0f).during(LibCore::Tween::Ms(healTrailDuration_secs_))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutCubic)));
    }

    void GaugeEffects::StartFullEffect()
    {
        fullTween_.Play(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(std::max(fullFlashDuration_secs_, fullShineDuration_secs_)))
            .via(LibCore::Tween::Ease(LibCore::EaseType::Linear)));
    }

    void GaugeEffects::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        if (const auto slider = FindSlider())
            UpdateTrail(slider->GetValue(), deltaTime);

        healTween_.Tick(deltaTime);
        fullTween_.Tick(deltaTime);

        if (gaugeFadeTween_.Tick(deltaTime))
            fadingOutGaugeSprite_.reset();

        if (isPulsing_)
            pulseTime_secs_ += deltaTime;
    }

    void GaugeEffects::DrawBand(const NanamiUi::Slider& slider, const float alongMin, const float alongMax, const float acrossMin, const float acrossMax, const unsigned int color) const
    {
        const glm::vec2 p1 = slider.FillToScreen(alongMin, acrossMin);
        const glm::vec2 p2 = slider.FillToScreen(alongMax, acrossMin);
        const glm::vec2 p3 = slider.FillToScreen(alongMax, acrossMax);
        const glm::vec2 p4 = slider.FillToScreen(alongMin, acrossMax);
        if (!slider.IsRotated())
        {
            const glm::vec2 min = glm::min(p1, p3);
            const glm::vec2 max = glm::max(p1, p3);
            DrawBox(
                static_cast<int>(std::round(min.x)),
                static_cast<int>(std::round(min.y)),
                static_cast<int>(std::round(max.x)),
                static_cast<int>(std::round(max.y)),
                color,
                TRUE);
            return;
        }

        DrawQuadrangleAA(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, color, TRUE);
    }

    void GaugeEffects::DrawTicks(const NanamiUi::Slider& slider) const
    {
        if (tickCount_ <= 1)
            return;

        const float acrossMin = bandInsetY_ + tickInsetY_;
        const float acrossMax = slider.AcrossLength() - bandInsetY_ - tickInsetY_;
        if (acrossMax <= acrossMin)
            return;

        // 目盛りは余白を除いた範囲を等分する
        const float startInset  = slider.GetFillStartInset();
        const float innerLength = std::max(0.0f, slider.AlongLength() - startInset - slider.GetFillEndInset());
        slider.ClipToDrawSize();
        for (int i = 1; i < tickCount_; ++i)
        {
            const float along = startInset + innerLength * static_cast<float>(i) / static_cast<float>(tickCount_);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickShadowAlpha_, 0, 255));
            DrawBand(slider, along, along + 1.0f, acrossMin, acrossMax, GetColor(0, 0, 0));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickHighlightAlpha_, 0, 255));
            DrawBand(slider, along + 1.0f, along + 2.0f, acrossMin, acrossMax, GetColor(255, 255, 255));
        }
    }

    void GaugeEffects::DrawTip(const NanamiUi::Slider& slider, const float value) const
    {
        if (!tipSprite_ || value <= 0.0f || value >= 1.0f || tipWidth_ <= 0.0f)
            return;

        const int tipHandle = tipSprite_->GetDxLibHandle();
        int imageW = 0;
        int imageH = 0;
        GetGraphSize(tipHandle, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0)
            return;

        // 始端からはみ出す分は画像の頭を切り落とす
        const float alongMax = slider.CalcFillLength(value);
        const float alongMin = std::max(0.0f, alongMax - tipWidth_);
        const int srcLeft    = std::clamp(static_cast<int>(std::lround((alongMin - (alongMax - tipWidth_)) / tipWidth_ * static_cast<float>(imageW))), 0, imageW);
        if (srcLeft >= imageW)
            return;

        const float acrossMin = bandInsetY_;
        const float acrossMax = slider.AcrossLength() - bandInsetY_;

        // 画像の横方向を伸びる方向に合わせる
        const glm::vec2 p1 = slider.FillToScreen(alongMin, acrossMin);
        const glm::vec2 p2 = slider.FillToScreen(alongMax, acrossMin);
        const glm::vec2 p3 = slider.FillToScreen(alongMax, acrossMax);
        const glm::vec2 p4 = slider.FillToScreen(alongMin, acrossMax);

        slider.ClipToDrawSize();
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        DrawRectModiGraphF(
            p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y,
            srcLeft, 0, imageW - srcLeft, imageH,
            tipHandle,
            TRUE);
    }

    void GaugeEffects::DrawHealTrail(const NanamiUi::Slider& slider, const float value) const
    {
        if (!healTween_.IsPlaying() || healTrailAlpha_ <= 0)
            return;

        const float progress = std::clamp(healTween_.Value(), 0.0f, 1.0f);
        const float lower    = std::lerp(healFrom_, value, progress);
        if (value <= lower)
            return;

        const float fade = 1.0f - progress;
        const ScopedDrawState drawState(false);

        // ゲージ画像そのものを色付きで加算して、模様を残したまま光らせる
        SetDrawBright(healTrailColor_.R(), healTrailColor_.G(), healTrailColor_.B());
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(std::clamp(healTrailAlpha_, 0, 255)) * fade));
        slider.DrawFillRange(slider.GetGaugeSprite()->GetDxLibHandle(), lower, value);
        SetDrawBright(255, 255, 255);

        // 伸びた先端に細い光の線
        if (healEdgeWidth_ > 0.0f && value < 1.0f)
        {
            const float edge      = slider.CalcFillLength(value);
            const float acrossMin = bandInsetY_;
            const float acrossMax = slider.AcrossLength() - bandInsetY_;
            slider.ClipToDrawSize();
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255.0f * fade));
            DrawBand(slider, std::max(0.0f, edge - healEdgeWidth_), edge, acrossMin, acrossMax, healTrailColor_.ToDxColor());
        }
    }

    void GaugeEffects::DrawFullEffect(const NanamiUi::Slider& slider, const float value) const
    {
        if (!fullTween_.IsPlaying() || value < fullValueThreshold)
            return;

        const float totalSecs = std::max(fullFlashDuration_secs_, fullShineDuration_secs_);
        const float elapsed   = std::clamp(fullTween_.Value(), 0.0f, 1.0f) * totalSecs;
        const ScopedDrawState drawState(false);

        // ゲージ全体を一瞬明るくして、すっと引かせる
        if (fullFlashAlpha_ > 0 && fullFlashDuration_secs_ > 0.0f && elapsed < fullFlashDuration_secs_)
        {
            const float fade = 1.0f - elapsed / fullFlashDuration_secs_;
            SetDrawBright(fullFlashColor_.R(), fullFlashColor_.G(), fullFlashColor_.B());
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(std::clamp(fullFlashAlpha_, 0, 255)) * fade * fade));
            slider.DrawFillRange(slider.GetGaugeSprite()->GetDxLibHandle(), 0.0f, 1.0f);
            SetDrawBright(255, 255, 255);
        }

        // 始端から終端へ、中央が明るい光の筋を流す
        if (fullShineAlpha_ > 0 && fullShineWidth_ > 0.0f && fullShineDuration_secs_ > 0.0f && elapsed < fullShineDuration_secs_)
        {
            constexpr int sliceCount = 12;
            const float t          = elapsed / fullShineDuration_secs_;
            const float eased      = 1.0f - (1.0f - t) * (1.0f - t);
            const float fillMin    = slider.CalcFillLength(0.0f);
            const float fillMax    = slider.CalcFillLength(1.0f);
            const float halfWidth  = fullShineWidth_ * 0.5f;
            const float center     = std::lerp(fillMin - halfWidth, fillMax + halfWidth, eased);
            const float sliceWidth = fullShineWidth_ / static_cast<float>(sliceCount);
            const float acrossMin  = bandInsetY_;
            const float acrossMax  = slider.AcrossLength() - bandInsetY_;

            slider.ClipToDrawSize();
            for (int i = 0; i < sliceCount; ++i)
            {
                const float sliceMin = center - halfWidth + sliceWidth * static_cast<float>(i);
                const float alongMin = std::max(fillMin, sliceMin);
                const float alongMax = std::min(fillMax, sliceMin + sliceWidth);
                if (alongMax <= alongMin)
                    continue;

                // 山なりの濃さ（中央 1、両端 0）
                const float offset = (static_cast<float>(i) + 0.5f) / static_cast<float>(sliceCount) * 2.0f - 1.0f;
                const float weight = 1.0f - offset * offset;
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(std::clamp(fullShineAlpha_, 0, 255)) * weight));
                DrawBand(slider, alongMin, alongMax, acrossMin, acrossMax, fullFlashColor_.ToDxColor());
            }
        }
    }

    void GaugeEffects::OnUserInterfaceRender()
    {
        const auto slider = FindSlider();
        if (!IsEnable() || !slider || !slider->IsEnable() || !slider->GetGaugeSprite())
            return;

        const float value = slider->GetValue();

        if (trailSprite_)
            slider->DrawFillRange(trailSprite_->GetDxLibHandle(), value, trailValue_);

        if (slider->IsStretchToDrawSize())
        {
            // 新しい画像は Slider が描いているので、古い画像を上に重ねて消していく
            if (fadingOutGaugeSprite_ && gaugeFadeDuration_secs_ > 0.0f)
            {
                const ScopedDrawState drawState(false);
                const float fadeOutRate = 1.0f - std::clamp(gaugeFadeTween_.Value(), 0.0f, 1.0f);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255.0f * fadeOutRate));
                slider->DrawFillRange(fadingOutGaugeSprite_->GetDxLibHandle(), 0.0f, value);
            }

            if (isPulsing_ && pulseMaxAlpha_ > 0)
            {
                const ScopedDrawState drawState(false);
                const float wave = 0.5f + 0.5f * std::sin(pulseTime_secs_ * pulseFrequency_hz_ * 2.0f * std::numbers::pi_v<float>);
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(pulseMaxAlpha_) * wave));
                slider->DrawFillRange(slider->GetGaugeSprite()->GetDxLibHandle(), 0.0f, value);
            }
        }

        DrawHealTrail(*slider, value);

        {
            const ScopedDrawState drawState(slider->IsStretchToDrawSize() && slider->IsRotated());
            DrawTicks(*slider);
            DrawTip(*slider, value);
        }

        DrawFullEffect(*slider, value);
    }

    void GaugeEffects::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
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
        ImGuiHelper::OnDrawInputField("healTrailColor_", healTrailColor_);
        ImGuiHelper::OnDrawInputField("healTrailAlpha_", healTrailAlpha_);
        ImGuiHelper::OnDrawInputField("healTrailHold_secs_", healTrailHold_secs_);
        ImGuiHelper::OnDrawInputField("healTrailDuration_secs_", healTrailDuration_secs_);
        ImGuiHelper::OnDrawInputField("healEdgeWidth_", healEdgeWidth_);
        ImGuiHelper::OnDrawInputField("fullFlashColor_", fullFlashColor_);
        ImGuiHelper::OnDrawInputField("fullFlashAlpha_", fullFlashAlpha_);
        ImGuiHelper::OnDrawInputField("fullFlashDuration_secs_", fullFlashDuration_secs_);
        ImGuiHelper::OnDrawInputField("fullShineWidth_", fullShineWidth_);
        ImGuiHelper::OnDrawInputField("fullShineAlpha_", fullShineAlpha_);
        ImGuiHelper::OnDrawInputField("fullShineDuration_secs_", fullShineDuration_secs_);
        ImGui::Text("trailValue_: %.3f  isPulsing_: %d  slider: %s", trailValue_, isPulsing_ ? 1 : 0, slider_.expired() ? "none" : "found");
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GaugeEffects);
#pragma endregion
