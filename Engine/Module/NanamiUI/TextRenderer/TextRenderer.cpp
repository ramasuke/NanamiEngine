#include "TextRenderer.h"
#include "../../GameObject/Transform/Transform.h"

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
    void TextRenderer::SetText(const std::string& text)
    {
        if (text_ == text) return;
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
    
    void TextRenderer::SetWorldMode(bool isWorld)
    {
        isWorldPos_ = isWorld;
    }
    
    void TextRenderer::UpdateTextTexture()
    {
        if (!isDirty_ || !fontFile_) return;
    
        if (textScreen_ == -1)
        {
            textScreen_ = MakeScreen(screenW_, screenH_, TRUE);
        }
    
        cachedSjis_ = Utf8ToShiftJis(text_);
    
        SetDrawScreen(textScreen_);
        ClearDrawScreen();
    
        DrawStringToHandle(
            0, 0,
            cachedSjis_.c_str(),
            textColor_.ToDxColor(),
            fontFile_->DxLibHandle()
        );
    
        SetDrawScreen(DX_SCREEN_BACK);
    
        isDirty_ = false;
    }

    void TextRenderer::OnDrawGui()
    {
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
    }

    void TextRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || !fontFile_) return;
    
        if (!isWorldPos_)
        {
            // UI描画
            DrawExtendStringFToHandle(
                Transform().GetWorldPos().x,
                Transform().GetWorldPos().y,
                Transform().GetWorldScale().x,
                Transform().GetWorldScale().y,
                Utf8ToShiftJis(text_).c_str(),
                textColor_.ToDxColor(),
                fontFile_->DxLibHandle()
            );
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
