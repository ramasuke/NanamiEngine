#include "NanamiUi_Slider.h"
#include <algorithm>
#include <cmath>
#include <numbers>

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../../Core/Application/Time/Time.h"

namespace
{
    // 描画範囲・ブレンドモード・描画モードを抜けるときに元へ戻す
    class ScopedDrawState final
    {
    public:
        explicit ScopedDrawState(const bool isBilinear)
            : drawMode_(GetDrawMode())
        {
            GetDrawBlendMode(&blendMode_, &blendParam_);
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
        }
        ScopedDrawState(const ScopedDrawState&) = delete;
        ScopedDrawState& operator=(const ScopedDrawState&) = delete;

    private:
        int drawMode_   = DX_DRAWMODE_NEAREST;
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

    void Slider::SetValueImmediate(const float value)
    {
        value_               = std::clamp(value, 0.0f, 1.0f);
        trailValue_          = value_;
        trailFrom_           = value_;
        trailWaitTimer_secs_ = 0.0f;
        trailElapsed_secs_   = 0.0f;
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

    Slider::DrawFrame Slider::CalcDrawFrame() const
    {
        constexpr float rotationEpsilon = 1.0e-4f;

        const auto worldPos  = Transform().GetWorldPos();
        const float angle    = glm::eulerAngles(Transform().GetWorldRot()).z;
        const bool isRotated = std::abs(angle) > rotationEpsilon;
        const float cosAngle = std::cos(angle);
        const float sinAngle = std::sin(angle);

        // 回転していないときは従来どおり整数座標に揃える
        const glm::vec2 origin(worldPos.x, worldPos.y);
        return DrawFrame{
            isRotated ? origin : glm::trunc(origin),
            glm::vec2(cosAngle, sinAngle),
            glm::vec2(-sinAngle, cosAngle),
            drawSize_,
            isRotated };
    }

    bool Slider::IsVerticalFill() const
    {
        return fillDirection_ == SliderFillDirection::BottomToTop || fillDirection_ == SliderFillDirection::TopToBottom;
    }

    float Slider::AlongLength() const
    {
        return IsVerticalFill() ? drawSize_.y : drawSize_.x;
    }

    float Slider::AcrossLength() const
    {
        return IsVerticalFill() ? drawSize_.x : drawSize_.y;
    }

    float Slider::CalcFillLength(const float fillRate) const
    {
        const float alongLength = AlongLength();
        if (fillRate <= 0.0f)
            return 0.0f;
        if (fillRate >= 1.0f)
            return alongLength;

        const float innerLength = std::max(0.0f, alongLength - fillStartInset_ - fillEndInset_);
        return std::min(alongLength, fillStartInset_ + innerLength * fillRate);
    }

    glm::vec2 Slider::FillToLocal(const float along, const float across) const
    {
        switch (fillDirection_)
        {
        case SliderFillDirection::RightToLeft: return glm::vec2(drawSize_.x - along, across);
        case SliderFillDirection::BottomToTop: return glm::vec2(across, drawSize_.y - along);
        case SliderFillDirection::TopToBottom: return glm::vec2(across, along);
        case SliderFillDirection::LeftToRight: break;
        }
        return glm::vec2(along, across);
    }

    Slider::LocalRect Slider::FillToLocalRect(const float alongMin, const float alongMax, const float acrossMin, const float acrossMax) const
    {
        const glm::vec2 a = FillToLocal(alongMin, acrossMin);
        const glm::vec2 b = FillToLocal(alongMax, acrossMax);
        return LocalRect{ glm::min(a, b), glm::max(a, b) };
    }

    void Slider::ClipToDrawSize(const DrawFrame& frame) const
    {
        if (frame.isRotated)
            return;

        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);
        SetDrawArea(x, y, x + static_cast<int>(frame.size.x), y + static_cast<int>(frame.size.y));
    }

    void Slider::DrawLayer(const DrawFrame& frame, const int graphHandle, const float fillRate) const
    {
        if (isStretchToDrawSize_)
            DrawStretchedLayer(frame, graphHandle, fillRate);
        else
            DrawUnstretchedLayer(frame, graphHandle, fillRate);
    }

    void Slider::DrawStretchedLayer(const DrawFrame& frame, const int graphHandle, const float fillRate) const
    {
        const float fillLength = CalcFillLength(fillRate);
        if (fillLength <= 0.0f)
            return;

        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);

        // 回転していなければ画面平行のクリップで px 単位に切れる
        if (!frame.isRotated)
        {
            const LocalRect visible = FillToLocalRect(0.0f, fillLength, 0.0f, AcrossLength());
            const int clipLeft   = x + static_cast<int>(visible.min.x);
            const int clipTop    = y + static_cast<int>(visible.min.y);
            const int clipRight  = x + static_cast<int>(visible.max.x);
            const int clipBottom = y + static_cast<int>(visible.max.y);
            if (clipRight <= clipLeft || clipBottom <= clipTop)
                return;

            SetDrawArea(clipLeft, clipTop, clipRight, clipBottom);
            DrawExtendGraphF(
                static_cast<float>(x),
                static_cast<float>(y),
                static_cast<float>(x + static_cast<int>(frame.size.x)),
                static_cast<float>(y + static_cast<int>(frame.size.y)),
                graphHandle,
                TRUE);
            return;
        }

        int imageW = 0;
        int imageH = 0;
        GetGraphSize(graphHandle, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0 || frame.size.x <= 0.0f || frame.size.y <= 0.0f)
            return;

        // 回転したクリップはできないので、元画像をテクセル単位で切り出して四隅に描く
        const float alongLength = AlongLength();
        const int imageAlong    = IsVerticalFill() ? imageH : imageW;
        const int texels        = std::clamp(static_cast<int>(std::lround(fillLength / alongLength * static_cast<float>(imageAlong))), 0, imageAlong);
        if (texels <= 0)
            return;

        const float drawnLength = alongLength * static_cast<float>(texels) / static_cast<float>(imageAlong);
        const LocalRect visible = FillToLocalRect(0.0f, drawnLength, 0.0f, AcrossLength());
        const float texelPerX   = static_cast<float>(imageW) / frame.size.x;
        const float texelPerY   = static_cast<float>(imageH) / frame.size.y;
        const int srcLeft   = static_cast<int>(std::lround(visible.min.x * texelPerX));
        const int srcTop    = static_cast<int>(std::lround(visible.min.y * texelPerY));
        const int srcRight  = static_cast<int>(std::lround(visible.max.x * texelPerX));
        const int srcBottom = static_cast<int>(std::lround(visible.max.y * texelPerY));
        if (srcRight <= srcLeft || srcBottom <= srcTop)
            return;

        const glm::vec2 p1 = frame.ToScreen(visible.min);
        const glm::vec2 p2 = frame.ToScreen(glm::vec2(visible.max.x, visible.min.y));
        const glm::vec2 p3 = frame.ToScreen(visible.max);
        const glm::vec2 p4 = frame.ToScreen(glm::vec2(visible.min.x, visible.max.y));
        DrawRectModiGraphF(
            p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y,
            srcLeft, srcTop, srcRight - srcLeft, srcBottom - srcTop,
            graphHandle,
            TRUE);
    }

    void Slider::DrawUnstretchedLayer(const DrawFrame& frame, const int graphHandle, const float fillRate) const
    {
        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);
        const int w = static_cast<int>(frame.size.x);
        const int h = static_cast<int>(frame.size.y);
        const int clipWidth = static_cast<int>(static_cast<float>(w) * std::clamp(fillRate, 0.0f, 1.0f));
        if (clipWidth <= 0)
            return;

        SetDrawArea(x, y, x + clipWidth, y + h);
        DrawRotaGraphF(
            static_cast<float>(x) + drawPosition_.x,
            static_cast<float>(y) + drawPosition_.y,
            Transform().GetWorldScale().x,
            Transform().GetWorldRot().z,
            graphHandle,
            TRUE);
    }

    void Slider::FillLocalRect(const DrawFrame& frame, const LocalRect& rect, const unsigned int color) const
    {
        if (!frame.isRotated)
        {
            const int x = static_cast<int>(frame.origin.x);
            const int y = static_cast<int>(frame.origin.y);
            DrawBox(
                x + static_cast<int>(std::round(rect.min.x)),
                y + static_cast<int>(std::round(rect.min.y)),
                x + static_cast<int>(std::round(rect.max.x)),
                y + static_cast<int>(std::round(rect.max.y)),
                color,
                TRUE);
            return;
        }

        const glm::vec2 p1 = frame.ToScreen(rect.min);
        const glm::vec2 p2 = frame.ToScreen(glm::vec2(rect.max.x, rect.min.y));
        const glm::vec2 p3 = frame.ToScreen(rect.max);
        const glm::vec2 p4 = frame.ToScreen(glm::vec2(rect.min.x, rect.max.y));
        DrawQuadrangleAA(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, color, TRUE);
    }

    void Slider::DrawTicks(const DrawFrame& frame) const
    {
        if (tickCount_ <= 1)
            return;

        const float acrossMin = bandInsetY_ + tickInsetY_;
        const float acrossMax = AcrossLength() - bandInsetY_ - tickInsetY_;
        if (acrossMax <= acrossMin)
            return;

        // 目盛りは余白を除いた範囲を等分する
        const float innerLength = std::max(0.0f, AlongLength() - fillStartInset_ - fillEndInset_);
        ClipToDrawSize(frame);
        for (int i = 1; i < tickCount_; ++i)
        {
            const float along = fillStartInset_ + innerLength * static_cast<float>(i) / static_cast<float>(tickCount_);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickShadowAlpha_, 0, 255));
            FillLocalRect(frame, FillToLocalRect(along, along + 1.0f, acrossMin, acrossMax), GetColor(0, 0, 0));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::clamp(tickHighlightAlpha_, 0, 255));
            FillLocalRect(frame, FillToLocalRect(along + 1.0f, along + 2.0f, acrossMin, acrossMax), GetColor(255, 255, 255));
        }
    }

    void Slider::DrawTip(const DrawFrame& frame) const
    {
        if (!tipSprite_ || value_ <= 0.0f || value_ >= 1.0f || tipWidth_ <= 0.0f)
            return;

        const int tipHandle = tipSprite_->GetDxLibHandle();
        int imageW = 0;
        int imageH = 0;
        GetGraphSize(tipHandle, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0)
            return;

        // 始端からはみ出す分は画像の頭を切り落とす
        const float alongMax = CalcFillLength(value_);
        const float alongMin = std::max(0.0f, alongMax - tipWidth_);
        const int srcLeft    = std::clamp(static_cast<int>(std::lround((alongMin - (alongMax - tipWidth_)) / tipWidth_ * static_cast<float>(imageW))), 0, imageW);
        if (srcLeft >= imageW)
            return;

        const float acrossMin = bandInsetY_;
        const float acrossMax = AcrossLength() - bandInsetY_;

        // 画像の横方向を伸びる方向に合わせる
        const glm::vec2 p1 = frame.ToScreen(FillToLocal(alongMin, acrossMin));
        const glm::vec2 p2 = frame.ToScreen(FillToLocal(alongMax, acrossMin));
        const glm::vec2 p3 = frame.ToScreen(FillToLocal(alongMax, acrossMax));
        const glm::vec2 p4 = frame.ToScreen(FillToLocal(alongMin, acrossMax));

        ClipToDrawSize(frame);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        DrawRectModiGraphF(
            p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y,
            srcLeft, 0, imageW - srcLeft, imageH,
            tipHandle,
            TRUE);
    }

    void Slider::OnUserInterfaceRender()
    {
        if (!IsEnable() || !gaugeSprite_)
            return;

        const DrawFrame frame = CalcDrawFrame();
        const bool isBilinear = isStretchToDrawSize_ && frame.isRotated;
        {
            const ScopedDrawState drawState(isBilinear);
            if (backgroundSprite_)
                DrawLayer(frame, backgroundSprite_->GetDxLibHandle(), 1.0f);
            if (trailSprite_)
                DrawLayer(frame, trailSprite_->GetDxLibHandle(), trailValue_);
        }

        if (!isStretchToDrawSize_)
            DrawMaskedGauge(frame);

        {
            const ScopedDrawState drawState(isBilinear);
            if (isStretchToDrawSize_)
            {
                const int gaugeHandle = gaugeSprite_->GetDxLibHandle();
                if (fadingOutGaugeSprite_ && gaugeFadeDuration_secs_ > 0.0f)
                {
                    const float fadeInRate = 1.0f - gaugeFadeTimer_secs_ / gaugeFadeDuration_secs_;
                    DrawLayer(frame, fadingOutGaugeSprite_->GetDxLibHandle(), value_);
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255.0f * std::clamp(fadeInRate, 0.0f, 1.0f)));
                    DrawLayer(frame, gaugeHandle, value_);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else
                {
                    DrawLayer(frame, gaugeHandle, value_);
                }

                if (isPulsing_ && pulseMaxAlpha_ > 0)
                {
                    const float wave = 0.5f + 0.5f * std::sin(pulseTime_secs_ * pulseFrequency_hz_ * 2.0f * std::numbers::pi_v<float>);
                    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(static_cast<float>(pulseMaxAlpha_) * wave));
                    DrawLayer(frame, gaugeHandle, value_);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
            }

            DrawTicks(frame);
            DrawTip(frame);
        }
    }

    void Slider::DrawMaskedGauge(const DrawFrame& frame) const
    {
        const int w = static_cast<int>(frame.size.x);
        const int h = static_cast<int>(frame.size.y);
        // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、直後の SetDrawScreen で完了待ちに入る
        const int useASyncLoad = GetUseASyncLoadFlag();
        SetUseASyncLoadFlag(FALSE);
        const int maskedScreen = MakeScreen(w, h, true);
        SetUseASyncLoadFlag(useASyncLoad);

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
        DrawGraph(static_cast<int>(frame.origin.x), static_cast<int>(frame.origin.y), maskedScreen, TRUE);
        DeleteGraph(maskedScreen);

    }

    void Slider::OnDrawGui()
    {
        ImGui::Text("Slider");

        if (ImGui::SliderFloat("value_", &value_, 0.0f, 1.0f))
            SetValueImmediate(value_);

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
        ImGuiHelper::OnDrawEnumField("fillDirection_", fillDirection_, SLIDER_FILL_DIRECTIONS, ToString);
        ImGuiHelper::OnDrawInputField("fillStartInset_", fillStartInset_);
        ImGuiHelper::OnDrawInputField("fillEndInset_", fillEndInset_);
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

        const DrawFrame frame = CalcDrawFrame();
        const glm::vec2 p1 = frame.ToScreen(glm::vec2(0.0f, 0.0f));
        const glm::vec2 p2 = frame.ToScreen(glm::vec2(frame.size.x, 0.0f));
        const glm::vec2 p3 = frame.ToScreen(frame.size);
        const glm::vec2 p4 = frame.ToScreen(glm::vec2(0.0f, frame.size.y));
        DrawQuadrangleAA(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, GetColor(255, 255, 255), FALSE);
    }
}
