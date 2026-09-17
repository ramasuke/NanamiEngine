#include "GameObjectMark.h"

#include <algorithm>
#include <string>

#include "ImGuiHelper.h"

namespace
{
    constexpr ImU32 MARK_COLORS[NanamiEngine::Module::GameObject::GAMEOBJECT_MARK_COLOR_COUNT] = {
        IM_COL32(128, 128, 128, 255),
        IM_COL32( 52, 120, 220, 255),
        IM_COL32( 40, 180, 180, 255),
        IM_COL32( 60, 175,  70, 255),
        IM_COL32(225, 190,  40, 255),
        IM_COL32(235, 130,  40, 255),
        IM_COL32(215,  55,  55, 255),
        IM_COL32(155,  80, 205, 255),
    };
    constexpr int   YELLOW_COLOR_INDEX   = 4;
    constexpr ImU32 OUTLINE_COLOR        = IM_COL32( 20,  20,  20, 220);
    constexpr ImU32 LIGHT_TEXT_COLOR     = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 DARK_TEXT_COLOR      = IM_COL32( 20,  20,  20, 255);
    constexpr float CIRCLE_RADIUS        = 6.0f;
    constexpr float DIAMOND_HALF_SIZE    = 7.0f;
    constexpr float LABEL_PADDING_X      = 6.0f;
    constexpr float LABEL_PADDING_Y      = 2.0f;
    constexpr float LABEL_MIN_TEXT_WIDTH = 8.0f;
    constexpr float PREVIEW_WIDTH        = 22.0f;
}

namespace NanamiEngine::Module::GameObject
{
    void DrawMark(ImDrawList& drawList, const ImVec2& center, const GameObjectMark mark, const char* label)
    {
        const int   colorIndex = ToColorIndex(mark);
        const ImU32 color      = MARK_COLORS[colorIndex];

        switch (ToShape(mark))
        {
        case GameObjectMarkShape::None:
            return;

        case GameObjectMarkShape::Label:
        {
            const ImVec2 textSize   = ImGui::CalcTextSize(label);
            const float  halfWidth  = (std::max)(textSize.x, LABEL_MIN_TEXT_WIDTH) * 0.5f + LABEL_PADDING_X;
            const float  halfHeight = textSize.y * 0.5f + LABEL_PADDING_Y;
            const ImVec2 rectMin(center.x - halfWidth, center.y - halfHeight);
            const ImVec2 rectMax(center.x + halfWidth, center.y + halfHeight);
            drawList.AddRectFilled(rectMin, rectMax, color, halfHeight);
            drawList.AddRect(rectMin, rectMax, OUTLINE_COLOR, halfHeight);
            const ImU32 textColor = colorIndex == YELLOW_COLOR_INDEX ? DARK_TEXT_COLOR : LIGHT_TEXT_COLOR;
            drawList.AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f), textColor, label);
            return;
        }

        case GameObjectMarkShape::Circle:
            drawList.AddCircleFilled(center, CIRCLE_RADIUS, color);
            drawList.AddCircle(center, CIRCLE_RADIUS, OUTLINE_COLOR);
            return;

        case GameObjectMarkShape::Diamond:
        {
            const ImVec2 top   (center.x,                     center.y - DIAMOND_HALF_SIZE);
            const ImVec2 right (center.x + DIAMOND_HALF_SIZE, center.y);
            const ImVec2 bottom(center.x,                     center.y + DIAMOND_HALF_SIZE);
            const ImVec2 left  (center.x - DIAMOND_HALF_SIZE, center.y);
            drawList.AddQuadFilled(top, right, bottom, left, color);
            drawList.AddQuad(top, right, bottom, left, OUTLINE_COLOR);
            return;
        }
        }
    }

    void DrawMarkPreviewGui(const GameObjectMark mark)
    {
        const float  height = ImGui::GetFrameHeight();
        const ImVec2 min    = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(PREVIEW_WIDTH, height));
        DrawMark(*ImGui::GetWindowDrawList(), ImVec2(min.x + PREVIEW_WIDTH * 0.5f, min.y + height * 0.5f), mark, "");
    }

    bool DrawChoiceMarkGui(const char* label, GameObjectMark& mark)
    {
        bool changed = false;

        if (ImGui::BeginCombo(label, ToName(mark)))
        {
            for (int i = 0; i < static_cast<int>(GameObjectMark::Count); ++i)
            {
                const GameObjectMark candidate = ToMark(i);
                const bool           selected  = candidate == mark;

                if (ImGui::Selectable((std::string("##") + ToName(candidate)).c_str(), selected))
                {
                    mark    = candidate;
                    changed = true;
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();

                // Selectable は行幅いっぱいに広がり SameLine で横に並べられないので、行の上に直接描く
                const ImVec2 itemMin = ImGui::GetItemRectMin();
                const ImVec2 itemMax = ImGui::GetItemRectMax();
                const float  centerY = (itemMin.y + itemMax.y) * 0.5f;
                ImDrawList&  drawList = *ImGui::GetWindowDrawList();
                DrawMark(drawList, ImVec2(itemMin.x + PREVIEW_WIDTH * 0.5f, centerY), candidate, "");
                drawList.AddText(ImVec2(itemMin.x + PREVIEW_WIDTH + ImGui::GetStyle().ItemInnerSpacing.x, itemMin.y),
                                 ImGui::GetColorU32(ImGuiCol_Text), ToName(candidate));
            }
            ImGui::EndCombo();
        }

        return changed;
    }
}
