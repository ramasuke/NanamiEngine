#include "TextRenderer.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::NanamiUi
{
    using LibCore::Dxlib::Utf8ToShiftJis;

    TextRenderer::~TextRenderer()
    {
        if (textScreen_ != -1)
        {
            DeleteGraph(textScreen_);
            textScreen_ = -1;
        }
    }

    void TextRenderer::SetText(const std::string& text)
    {
        if (text_ == text)
            return;
        
        text_ = text;
        isDirty_ = true;
    }
    
    void TextRenderer::SetFont(const std::shared_ptr<Asset::TtfFontFile>& font)
    {
        fontFile_ = font;
        isDirty_ = true;
    }
    
    void TextRenderer::SetTextColor(const Color32& color)
    {
        textColor_ = color;
        isDirty_ = true;
    }
    
    void TextRenderer::SetWorldMode(const bool isWorld)
    {
        isWorldPos_ = isWorld;
    }

    void TextRenderer::SetTextAlign(const TextAlign align)
    {
        textAlign_ = align;
        isDirty_ = true;
    }

    void TextRenderer::SetBlendRate(const int blendRate)
    {
        blendRate_ = std::clamp(blendRate, 0, 255);
    }

    float TextRenderer::MeasureTextWidth() const
    {
        const auto font = fontFile_.get();
        if (!font || text_.empty())
            return 0.0f;
        const int fontHandle = font->HandleForPixelSize(std::max(1, font->Size()));
        if (fontHandle == -1)
            return 0.0f;

        std::istringstream ss(Utf8ToShiftJis(text_));
        std::string line;
        int maxWidth = 0;
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            maxWidth = std::max(maxWidth, GetDrawStringWidthToHandle(line.c_str(), static_cast<int>(line.size()), fontHandle));
        }
        return static_cast<float>(maxWidth);
    }

    void TextRenderer::UpdateTextTexture()
    {
        if (!isDirty_ || !fontFile_)
            return;

        cachedSjis_ = Utf8ToShiftJis(text_);

        const int fontHandle = fontFile_->DxLibHandle();
        const int lineHeight = GetFontSizeToHandle(fontHandle);

        // テキストの実サイズを計算
        std::vector<std::string> lines;
        std::vector<int> lineWidths;
        int newW = 1;
        std::istringstream ss(cachedSjis_);
        std::string line;
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            const int lineW = GetDrawStringWidthToHandle(line.c_str(), static_cast<int>(line.size()), fontHandle);
            newW = std::max(newW, lineW);
            lineWidths.push_back(lineW);
            lines.push_back(std::move(line));
        }
        if (lines.empty())
        {
            lines.emplace_back();
            lineWidths.push_back(0);
        }
        // 縁取りがはみ出して切れないよう上下左右に余白を取る
        const int edge = std::max(GetFontEdgeSizeToHandle(fontHandle), 0);
        const int newH = std::max(lineHeight * static_cast<int>(lines.size()), 1) + edge * 2;
        newW = std::max(newW, 1) + edge * 2;

        // サイズが変わった場合は古いスクリーンを解放して再生成
        if (textScreen_ != -1 && (screenW_ != newW || screenH_ != newH))
        {
            DeleteGraph(textScreen_);
            textScreen_ = -1;
        }
        screenW_ = newW;
        screenH_ = newH;

        if (textScreen_ == -1)
        {
            // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、直後の SetDrawScreen で完了待ちに入る
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            textScreen_ = MakeScreen(screenW_, screenH_, TRUE);
            SetUseASyncLoadFlag(useASyncLoad);
        }

        SetDrawScreen(textScreen_);
        ClearDrawScreen();
        const float alignFactor = ToAlignFactor(textAlign_);
        const int contentW = newW - edge * 2;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const int lineX = edge + static_cast<int>((contentW - lineWidths[i]) * alignFactor);
            const int lineY = edge + static_cast<int>(i) * lineHeight;
            DrawStringToHandle(lineX, lineY, lines[i].c_str(), textColor_.ToDxColor(), fontHandle, fontFile_->EdgeColor().ToDxColor());
        }
        SetDrawScreen(DX_SCREEN_BACK);

        isDirty_ = false;
    }

    void TextRenderer::OnDrawGui()
    {
        if (ImGui::Button("UpdateDisplayText"))
        {
            isDirty_ = true;
        }
        ImGui::Checkbox("isWorldPos_", &isWorldPos_);

        ImGuiHelper::OnDrawInputField("fontFile_", fontFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);

        std::vector<char> buffer(1024);
        strncpy_s(buffer.data(), buffer.size(), text_.c_str(), _TRUNCATE);

        if (ImGui::InputTextMultiline("text_", buffer.data(), buffer.size()))
        {
            text_ = buffer.data();
            isDirty_ = true;
        }

        ImGuiHelper::OnDrawInputField("textColor_", textColor_);
        ImGuiHelper::OnDrawEnumField("textAlign_", textAlign_, TEXT_ALIGNS, ToString);

        if (isWorldPos_)
        {
            ImGui::Text("screenW_: %d  screenH_: %d", screenW_, screenH_);
        }
    }

    void TextRenderer::DrawScreenText() const
    {
        const float x = Transform().GetWorldPos().x;
        const float y = Transform().GetWorldPos().y;
        const auto font = fontFile_.get();

        // 基準サイズのフォントを縮めて描くと最近傍補間で細い線が欠けるので、画面上の大きさのハンドルで描き、
        // 残りの端数(スケールのアニメ中など)だけを拡大率として渡す
        const float fontSize = static_cast<float>(font->Size());
        const float pixelSizeY = fontSize * Transform().GetWorldScale().y;
        if (pixelSizeY <= 0.0f)
            return;
        const int fontHandle = font->HandleForPixelSize(std::max(1, static_cast<int>(std::lround(pixelSizeY))));
        if (fontHandle == -1)
            return;
        const float handleSize = static_cast<float>(GetFontSizeToHandle(fontHandle));
        const float scaleX = fontSize * Transform().GetWorldScale().x / handleSize;
        const float scaleY = pixelSizeY / handleSize;
        const int dxColor = textColor_.ToDxColor();
        const int edgeDxColor = font->EdgeColor().ToDxColor();
        const std::string sjis = Utf8ToShiftJis(text_);

        if (textAlign_ == TextAlign::Left)
        {
            DrawExtendStringFToHandle(
                x, y,
                scaleX, scaleY,
                sjis.c_str(),
                dxColor,
                fontHandle,
                edgeDxColor
            );
            return;
        }

        const float alignFactor = ToAlignFactor(textAlign_);
        const float lineHeight = handleSize * scaleY;

        std::istringstream ss(sjis);
        std::string line;
        int lineIndex = 0;
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            const int lineW = GetDrawExtendStringWidthToHandle(scaleX, line.c_str(), static_cast<int>(line.size()), fontHandle);
            const float lineX = x - static_cast<float>(lineW) * alignFactor;
            const float lineY = y + static_cast<float>(lineIndex) * lineHeight;

            DrawExtendStringFToHandle(
                lineX, lineY,
                scaleX, scaleY,
                line.c_str(),
                dxColor,
                fontHandle,
                edgeDxColor
            );
            ++lineIndex;
        }
    }

    void TextRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !fontFile_ || blendRate_ <= 0) return;

        const bool isTranslucent = blendRate_ < 255;
        if (isTranslucent)
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blendRate_);

        if (!isWorldPos_)
        {
            DrawScreenText();
        }
        else
        {
            UpdateTextTexture();
    
            VECTOR pos = VGet(
                Transform().GetWorldPos().x,
                Transform().GetWorldPos().y,
                Transform().GetWorldPos().z
            );

            SetUseZBuffer3D  (FALSE);
            SetWriteZBuffer3D(FALSE);

            DrawBillboard3D(
                pos,
                0.5f, 0.5f,
                Transform().GetWorldScale().z,
                0.0f,
                textScreen_,
                TRUE
            );

            SetUseZBuffer3D  (TRUE);
            SetWriteZBuffer3D(TRUE);
        }

        if (isTranslucent)
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::TextRenderer);
#pragma endregion
