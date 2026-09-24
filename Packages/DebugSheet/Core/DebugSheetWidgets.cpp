#include "DebugSheetWidgets.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "DebugSheetStyle.h"

namespace NanamiEngine::DebugSheet::Widgets
{
    namespace
    {
        constexpr double CONFIRM_TIMEOUT_secs = 3.0;

        ImU32 ToU32(const Palette::Rgba& color)
        {
            return ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, color.a));
        }

        ImVec4 ToImVec4(const Palette::Rgba& color)
        {
            return { color.r, color.g, color.b, color.a };
        }

        ImVec2 CellSize()
        {
            return { -FLT_MIN, Metrics::CELL_HEIGHT };
        }

        /** @brief 直前のセルの右端、縦中央に文字を置く */
        void DrawTrailingText(const char* text, const Palette::Rgba& color)
        {
            const ImVec2 min  = ImGui::GetItemRectMin();
            const ImVec2 max  = ImGui::GetItemRectMax();
            const ImVec2 size = ImGui::CalcTextSize(text);
            const ImVec2 position(max.x - size.x - 14.0f, min.y + (max.y - min.y - size.y) * 0.5f);
            ImGui::GetWindowDrawList()->AddText(position, ToU32(color), text);
        }
    }

    void Header(const std::string_view text)
    {
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::TextColored(ToImVec4(Palette::ACCENT), "%.*s", static_cast<int>(text.size()), text.data());

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float  width  = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddLine(cursor, ImVec2(cursor.x + width, cursor.y), ToU32(Palette::CELL_HOVERED), 1.0f);
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
    }

    void Note(const std::string_view text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(Palette::TEXT_DIM));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    bool Button(const std::string_view label)
    {
        return ImGui::Button(std::string(label).c_str(), CellSize());
    }

    bool NavigationCell(const std::string_view label)
    {
        const bool pressed = Button(label);
        DrawTrailingText(">", Palette::ACCENT);
        return pressed;
    }

    bool Toggle(const std::string_view label, bool& value)
    {
        const bool pressed = Button(label);
        if (pressed)
            value = !value;

        // NOTE: 右端に丸いスイッチを描く
        const ImVec2 min      = ImGui::GetItemRectMin();
        const ImVec2 max      = ImGui::GetItemRectMax();
        constexpr float WIDTH  = 44.0f;
        constexpr float HEIGHT = 22.0f;
        const ImVec2 pillMin(max.x - WIDTH - 12.0f, min.y + (max.y - min.y - HEIGHT) * 0.5f);
        const ImVec2 pillMax(pillMin.x + WIDTH, pillMin.y + HEIGHT);
        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(pillMin, pillMax, ToU32(value ? Palette::ACCENT : Palette::BACKGROUND), HEIGHT * 0.5f);

        const float  radius = HEIGHT * 0.5f - 3.0f;
        const ImVec2 knob(value ? pillMax.x - HEIGHT * 0.5f : pillMin.x + HEIGHT * 0.5f, pillMin.y + HEIGHT * 0.5f);
        drawList->AddCircleFilled(knob, radius, ToU32(Palette::TEXT));
        return pressed;
    }

    void Label(const std::string_view key, const std::string_view value)
    {
        const float lineStart = ImGui::GetCursorPosX();
        const float right     = lineStart + ImGui::GetContentRegionAvail().x;

        ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(Palette::TEXT_DIM));
        ImGui::TextUnformatted(key.data(), key.data() + key.size());
        ImGui::PopStyleColor();

        const float valueWidth = ImGui::CalcTextSize(value.data(), value.data() + value.size()).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), right - valueWidth));
        ImGui::TextUnformatted(value.data(), value.data() + value.size());
    }

    int ButtonRow(const std::string_view label, const std::initializer_list<const char*> buttons)
    {
        const std::string text(label);
        ImGui::PushID(text.c_str());

        const float lineStart = ImGui::GetCursorPosX();
        const float right     = lineStart + ImGui::GetContentRegionAvail().x;
        const auto& style     = ImGui::GetStyle();

        float buttonsWidth = 0.0f;
        for (const char* button : buttons)
            buttonsWidth += ImGui::CalcTextSize(button).x + style.FramePadding.x * 2.0f + style.ItemSpacing.x;
        buttonsWidth -= style.ItemSpacing.x;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text.c_str());

        int pressed = -1;
        int index   = 0;
        for (const char* button : buttons)
        {
            ImGui::SameLine();
            if (index == 0)
                ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), right - buttonsWidth));

            ImGui::PushID(index);
            if (ImGui::Button(button))
                pressed = index;
            ImGui::PopID();
            ++index;
        }

        ImGui::PopID();
        return pressed;
    }

    bool ConfirmButton(const std::string_view label, const std::string_view id)
    {
        static std::unordered_map<std::string, double> armedUntil;

        const std::string key(id);
        const double      now     = ImGui::GetTime();
        const auto        found   = armedUntil.find(key);
        const bool        isArmed = found != armedUntil.end() && found->second > now;

        if (isArmed)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,        ToImVec4(Palette::DANGER));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ToImVec4(Palette::DANGER));
        }

        const std::string text    = isArmed ? "もう一度押すと実行: " + std::string(label) : std::string(label);
        const bool        pressed = ImGui::Button((text + "##" + key).c_str(), CellSize());

        if (isArmed)
            ImGui::PopStyleColor(2);

        if (!pressed)
            return false;

        if (isArmed)
        {
            armedUntil.erase(key);
            return true;
        }

        armedUntil[key] = now + CONFIRM_TIMEOUT_secs;
        return false;
    }

    bool InputInt(const std::string_view label, int& value, const int step)
    {
        const std::string text(label);
        ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(Palette::TEXT_DIM));
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopStyleColor();

        ImGui::SetNextItemWidth(-FLT_MIN);
        return ImGui::InputInt(("##" + text).c_str(), &value, step, step * 10);
    }
}
