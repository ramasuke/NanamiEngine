#pragma once
#include <array>
#include <string_view>

namespace NanamiEngine::Module::NanamiUi
{
    enum class TextAlign : int
    {
        Left = 0,
        Center = 1,
        Right = 2,
    };

    constexpr std::array TEXT_ALIGNS
    {
        TextAlign::Left,
        TextAlign::Center,
        TextAlign::Right,
    };

    constexpr std::string_view ToString(const TextAlign align)
    {
        switch (align)
        {
        case TextAlign::Left:   return "Left";
        case TextAlign::Center: return "Center";
        case TextAlign::Right:  return "Right";
        }
        return "Unknown";
    }

    constexpr float ToAlignFactor(const TextAlign align)
    {
        switch (align)
        {
        case TextAlign::Left:   return 0.0f;
        case TextAlign::Center: return 0.5f;
        case TextAlign::Right:  return 1.0f;
        }
        return 0.0f;
    }
}
