#pragma once

namespace NanamiEngine::DebugSheet
{
    /** @brief シート専用の配色。エディタの ImGui と見分けがつくよう濃紺 + オレンジにする */
    namespace Palette
    {
        struct Rgba { float r, g, b, a; };

        constexpr Rgba BACKGROUND   { 0.06f, 0.08f, 0.13f, 0.95f };
        constexpr Rgba CELL         { 0.12f, 0.15f, 0.22f, 1.00f };
        constexpr Rgba CELL_HOVERED { 0.18f, 0.22f, 0.31f, 1.00f };
        constexpr Rgba CELL_ACTIVE  { 0.55f, 0.30f, 0.10f, 1.00f };
        constexpr Rgba ACCENT       { 1.00f, 0.56f, 0.16f, 1.00f };
        constexpr Rgba TEXT         { 0.93f, 0.94f, 0.96f, 1.00f };
        constexpr Rgba TEXT_DIM     { 0.52f, 0.58f, 0.68f, 1.00f };
        constexpr Rgba DANGER       { 0.86f, 0.28f, 0.28f, 1.00f };
        constexpr Rgba SUCCESS      { 0.36f, 0.78f, 0.52f, 1.00f };
    }

    namespace Metrics
    {
        constexpr float PANEL_WIDTH  = 440.0f;
        constexpr float PANEL_MARGIN = 12.0f;
        constexpr float CELL_HEIGHT  = 40.0f;
        constexpr float FONT_SCALE   = 1.3f;
    }

    /** @brief 生存中だけシートのスタイルを積む。エディタ側のスタイルは壊さない */
    class ScopedStyle final
    {
    public:
        ScopedStyle();
        ~ScopedStyle();
        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle& operator=(const ScopedStyle&) = delete;

    private:
        int colorCount_ = 0;
        int varCount_   = 0;
    };
}
