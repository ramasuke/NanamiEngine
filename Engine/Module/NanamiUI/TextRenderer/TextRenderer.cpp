#include "TextRenderer.h"
#include "../../GameObject/Transform/Transform.h"
#include <sstream>
#include <vector>

std::string Utf8ToShiftJis(const std::string& utf8)
{
    int wideSize = MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.c_str(), -1,
        nullptr, 0
    );

    std::wstring wide(wideSize, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.c_str(), -1,
        wide.data(), wideSize
    );

    int sjisSize = WideCharToMultiByte(
        932, 0,
        wide.c_str(), -1,
        nullptr, 0,
        nullptr, nullptr
    );

    std::string sjis(sjisSize, '\0');
    WideCharToMultiByte(
        932, 0,
        wide.c_str(), -1,
        sjis.data(), sjisSize,
        nullptr, nullptr
    );

    return sjis;
}

namespace NanamiEngine::Module::NanamiUi
{
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

    void TextRenderer::UpdateTextTexture()
    {
        if (!isDirty_ || !fontFile_)
            return;

        cachedSjis_ = Utf8ToShiftJis(text_);

        const int fontHandle = fontFile_->DxLibHandle();
        const int lineHeight = GetFontSizeToHandle(fontHandle);

        // テキストの実サイズを計算（複数行対応）
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
        const int newH = std::max(lineHeight * static_cast<int>(lines.size()), 1);
        newW = std::max(newW, 1);

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
            textScreen_ = MakeScreen(screenW_, screenH_, TRUE);
        }

        SetDrawScreen(textScreen_);
        ClearDrawScreen();
        const float alignFactor = ToAlignFactor(textAlign_);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const int lineX = static_cast<int>((newW - lineWidths[i]) * alignFactor);
            DrawStringToHandle(lineX, static_cast<int>(i) * lineHeight, lines[i].c_str(), textColor_.ToDxColor(), fontHandle);
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

    void TextRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !fontFile_) return;
    
        if (!isWorldPos_)
        {
            const float x = Transform().GetWorldPos().x;
            const float y = Transform().GetWorldPos().y;
            const float scaleX = Transform().GetWorldScale().x;
            const float scaleY = Transform().GetWorldScale().y;
            const int fontHandle = fontFile_->DxLibHandle();
            const std::string sjis = Utf8ToShiftJis(text_);

            if (textAlign_ == TextAlign::Left)
            {
                DrawExtendStringFToHandle(
                    x, y,
                    scaleX, scaleY,
                    sjis.c_str(),
                    textColor_.ToDxColor(),
                    fontHandle
                );
            }
            else
            {
                const float alignFactor = ToAlignFactor(textAlign_);
                const float lineHeight = static_cast<float>(GetFontSizeToHandle(fontHandle)) * scaleY;

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
                        textColor_.ToDxColor(),
                        fontHandle
                    );
                    ++lineIndex;
                }
            }
        }
        else
        {
            UpdateTextTexture();
    
            VECTOR pos = VGet(
                Transform().GetWorldPos().x,
                Transform().GetWorldPos().y,
                Transform().GetWorldPos().z
            );
    
            DrawBillboard3D(
                pos,
                0.5f, 0.5f,
                Transform().GetWorldScale().z,
                0.0f,
                textScreen_,
                TRUE
            );
        }
    }
}
