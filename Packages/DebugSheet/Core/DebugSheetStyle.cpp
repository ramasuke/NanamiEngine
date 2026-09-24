#include "DebugSheetStyle.h"

namespace NanamiEngine::DebugSheet
{
    namespace
    {
        ImVec4 ToImVec4(const Palette::Rgba& color)
        {
            return { color.r, color.g, color.b, color.a };
        }
    }

    ScopedStyle::ScopedStyle()
    {
        const auto pushColor = [this](const ImGuiCol index, const Palette::Rgba& color)
        {
            ImGui::PushStyleColor(index, ToImVec4(color));
            ++colorCount_;
        };
        const auto pushVar = [this]<class T>(const ImGuiStyleVar index, const T value)
        {
            ImGui::PushStyleVar(index, value);
            ++varCount_;
        };

        pushColor(ImGuiCol_WindowBg,          Palette::BACKGROUND);
        pushColor(ImGuiCol_ChildBg,           Palette::BACKGROUND);
        pushColor(ImGuiCol_Border,            Palette::CELL_HOVERED);
        pushColor(ImGuiCol_Text,              Palette::TEXT);
        pushColor(ImGuiCol_TextDisabled,      Palette::TEXT_DIM);
        pushColor(ImGuiCol_Button,            Palette::CELL);
        pushColor(ImGuiCol_ButtonHovered,     Palette::CELL_HOVERED);
        pushColor(ImGuiCol_ButtonActive,      Palette::CELL_ACTIVE);
        pushColor(ImGuiCol_FrameBg,           Palette::CELL);
        pushColor(ImGuiCol_FrameBgHovered,    Palette::CELL_HOVERED);
        pushColor(ImGuiCol_FrameBgActive,     Palette::CELL_ACTIVE);
        pushColor(ImGuiCol_Separator,         Palette::CELL_HOVERED);
        pushColor(ImGuiCol_ScrollbarBg,       Palette::BACKGROUND);
        pushColor(ImGuiCol_ScrollbarGrab,     Palette::CELL_HOVERED);
        pushColor(ImGuiCol_CheckMark,         Palette::ACCENT);
        pushColor(ImGuiCol_SliderGrab,        Palette::ACCENT);
        pushColor(ImGuiCol_TextSelectedBg,    Palette::CELL_ACTIVE);
        pushColor(ImGuiCol_Header,            Palette::CELL);
        pushColor(ImGuiCol_HeaderHovered,     Palette::CELL_HOVERED);
        pushColor(ImGuiCol_HeaderActive,      Palette::CELL_ACTIVE);

        pushVar(ImGuiStyleVar_WindowRounding,    12.0f);
        pushVar(ImGuiStyleVar_WindowBorderSize,  1.0f);
        pushVar(ImGuiStyleVar_WindowPadding,     ImVec2(14.0f, 14.0f));
        pushVar(ImGuiStyleVar_FrameRounding,     8.0f);
        pushVar(ImGuiStyleVar_FramePadding,      ImVec2(12.0f, 8.0f));
        pushVar(ImGuiStyleVar_ItemSpacing,       ImVec2(8.0f, 6.0f));
        pushVar(ImGuiStyleVar_ScrollbarSize,     10.0f);
        pushVar(ImGuiStyleVar_ScrollbarRounding, 8.0f);
        pushVar(ImGuiStyleVar_ButtonTextAlign,   ImVec2(0.0f, 0.5f));
    }

    ScopedStyle::~ScopedStyle()
    {
        ImGui::PopStyleVar(varCount_);
        ImGui::PopStyleColor(colorCount_);
    }
}
