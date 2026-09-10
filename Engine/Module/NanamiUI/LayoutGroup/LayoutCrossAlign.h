#pragma once
#include <array>
#include <string_view>

namespace NanamiEngine::Module::NanamiUi
{
    // HorizontalLayoutGroup/VerticalLayoutGroupの積み重ね軸と直交する軸(cross axis)の揃え方。
    // このエンジンにはRectTransformが無く、各UIコンポーネントは自身のTransform位置を
    // 要素の中心として扱う(Button::eventAreaSize_等参照)ため、Start/Endは
    // cellSize_の半分だけ中心をオフセットすることで「揃える」。
    enum class LayoutCrossAlign : int
    {
        Start = 0,
        Center = 1,
        End = 2,
    };

    constexpr std::array LAYOUT_CROSS_ALIGNS
    {
        LayoutCrossAlign::Start,
        LayoutCrossAlign::Center,
        LayoutCrossAlign::End,
    };

    constexpr std::string_view ToString(const LayoutCrossAlign align)
    {
        switch (align)
        {
        case LayoutCrossAlign::Start:  return "Start";
        case LayoutCrossAlign::Center: return "Center";
        case LayoutCrossAlign::End:    return "End";
        }
        return "Unknown";
    }

    constexpr float ToCrossAlignFactor(const LayoutCrossAlign align)
    {
        switch (align)
        {
        case LayoutCrossAlign::Start:  return 0.5f;
        case LayoutCrossAlign::Center: return 0.0f;
        case LayoutCrossAlign::End:    return -0.5f;
        }
        return 0.0f;
    }
}
