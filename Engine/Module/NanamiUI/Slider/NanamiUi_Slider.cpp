#include "NanamiUi_Slider.h"
#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

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
}

namespace NanamiEngine::Module::NanamiUi
{
    void Slider::SetValue(const float value)
    {
        value_ = std::clamp(value, 0.0f, 1.0f);
    }

    void Slider::DrawFillRange(const int graphHandle, const float fromRate, const float toRate) const
    {
        const DrawFrame frame = CalcDrawFrame();
        const ScopedDrawState drawState(isStretchToDrawSize_ && frame.isRotated);
        DrawLayer(frame, graphHandle, fromRate, toRate);
    }

    glm::vec2 Slider::FillToScreen(const float along, const float across) const
    {
        return CalcDrawFrame().ToScreen(FillToLocal(along, across));
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

    void Slider::ClipToDrawSize() const
    {
        const DrawFrame frame = CalcDrawFrame();
        if (frame.isRotated)
            return;

        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);
        SetDrawArea(x, y, x + static_cast<int>(frame.size.x), y + static_cast<int>(frame.size.y));
    }

    void Slider::DrawLayer(const DrawFrame& frame, const int graphHandle, const float fromRate, const float toRate) const
    {
        if (toRate <= fromRate)
            return;
        if (isStretchToDrawSize_)
            DrawStretchedLayer(frame, graphHandle, fromRate, toRate);
        else
            DrawUnstretchedLayer(frame, graphHandle, fromRate, toRate);
    }

    void Slider::DrawStretchedLayer(const DrawFrame& frame, const int graphHandle, const float fromRate, const float toRate) const
    {
        const float startLength = CalcFillLength(fromRate);
        const float fillLength  = CalcFillLength(toRate);
        if (fillLength <= startLength)
            return;

        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);

        // 回転していなければ画面平行のクリップで px 単位に切れる
        if (!frame.isRotated)
        {
            const LocalRect visible = FillToLocalRect(startLength, fillLength, 0.0f, AcrossLength());
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
        const auto toTexel      = [&](const float length) { return std::clamp(static_cast<int>(std::lround(length / alongLength * static_cast<float>(imageAlong))), 0, imageAlong); };
        const int startTexel    = toTexel(startLength);
        const int endTexel      = toTexel(fillLength);
        if (endTexel <= startTexel)
            return;

        const float texelLength = alongLength / static_cast<float>(imageAlong);
        const LocalRect visible = FillToLocalRect(texelLength * static_cast<float>(startTexel), texelLength * static_cast<float>(endTexel), 0.0f, AcrossLength());
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

    void Slider::DrawUnstretchedLayer(const DrawFrame& frame, const int graphHandle, const float fromRate, const float toRate) const
    {
        const int x = static_cast<int>(frame.origin.x);
        const int y = static_cast<int>(frame.origin.y);
        const int w = static_cast<int>(frame.size.x);
        const int h = static_cast<int>(frame.size.y);
        const int clipLeft  = static_cast<int>(static_cast<float>(w) * std::clamp(fromRate, 0.0f, 1.0f));
        const int clipRight = static_cast<int>(static_cast<float>(w) * std::clamp(toRate, 0.0f, 1.0f));
        if (clipRight <= clipLeft)
            return;

        SetDrawArea(x + clipLeft, y, x + clipRight, y + h);
        DrawRotaGraphF(
            static_cast<float>(x) + drawPosition_.x,
            static_cast<float>(y) + drawPosition_.y,
            Transform().GetWorldScale().x,
            Transform().GetWorldRot().z,
            graphHandle,
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
                DrawLayer(frame, backgroundSprite_->GetDxLibHandle(), 0.0f, 1.0f);
            if (isStretchToDrawSize_)
                DrawLayer(frame, gaugeSprite_->GetDxLibHandle(), 0.0f, value_);
        }

        if (!isStretchToDrawSize_)
            DrawMaskedGauge(frame);
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

        ImGui::SliderFloat("value_", &value_, 0.0f, 1.0f);

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

        const DrawFrame frame = CalcDrawFrame();
        const glm::vec2 p1 = frame.ToScreen(glm::vec2(0.0f, 0.0f));
        const glm::vec2 p2 = frame.ToScreen(glm::vec2(frame.size.x, 0.0f));
        const glm::vec2 p3 = frame.ToScreen(frame.size);
        const glm::vec2 p4 = frame.ToScreen(glm::vec2(0.0f, frame.size.y));
        DrawQuadrangleAA(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, GetColor(255, 255, 255), FALSE);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiUi::Slider);
#pragma endregion
