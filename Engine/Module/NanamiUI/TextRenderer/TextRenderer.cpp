#include "TextRenderer.h"

#include "../../GameObject/Transform/Transform.h"

std::string UTF8ToShiftJIS(const std::string& utf8)
{
    // UTF-8 → UTF-16
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

    // UTF-16 → Shift-JIS
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


void NanamiUi::TextRenderer::SetText(const std::string& text)
{
    text_ = text;
}

void NanamiUi::TextRenderer::SetFont(const std::shared_ptr<Asset::TtfFontFile>& font)
{
    fontFile_ = font;
}

void NanamiUi::TextRenderer::SetTextColor(const Color32& color)
{
    textColor_ = color;
}

void NanamiUi::TextRenderer::OnUserInterfaceRender()
{
    if (!IsEnable())
        return;
    
    if (fontFile_)
    {
        DrawExtendStringFToHandle(
            Transform().GetWorldPos().x,
            Transform().GetWorldPos().y,
            Transform().GetWorldScale().x,
            Transform().GetWorldScale().y,
            UTF8ToShiftJIS(text_).c_str(),
            textColor_.ToDxColor(),
            fontFile_->DxLibHandle()
        );
    }
}

void NanamiUi::TextRenderer::OnDrawGui()
{
    ImGui::Text("%s", std::to_string(IsEnable()).c_str());

    if (ImGui::Button("ChangeIsEnable"))
    {
        SetEnable(!IsEnable());
    }

    ImGuiHelper::OnDrawInputField("fontFile_", fontFile_);
    ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);

    std::vector<char> buffer(1024);

    strncpy_s(
        buffer.data(),
        buffer.size(),
        text_.c_str(),
        _TRUNCATE
    );
    buffer.back() = '\0';

    if (ImGui::InputTextMultiline(
            "text_",
            buffer.data(),
            buffer.size(),
            ImVec2(-FLT_MIN, 100.0f)))
    {
        text_ = buffer.data();
    }

    ImGuiHelper::OnDrawInputField("textColor_", textColor_);
}