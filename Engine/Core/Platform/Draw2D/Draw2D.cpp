#include "Draw2D.h"

#include <algorithm>

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"

namespace NanamiEngine::Platform::Draw2D
{
    namespace
    {
        static_assert(static_cast<int>(FilterMode::Nearest)     == DX_DRAWMODE_NEAREST);
        static_assert(static_cast<int>(FilterMode::Bilinear)    == DX_DRAWMODE_BILINEAR);
        static_assert(static_cast<int>(FilterMode::Anisotropic) == DX_DRAWMODE_ANISOTROPIC);

        int ToFlag(const bool value) { return value ? TRUE : FALSE; }
    }

    BlendState GetBlendMode()
    {
        int mode = DX_BLENDMODE_NOBLEND, param = 0;
        GetDrawBlendMode(&mode, &param);
        return { static_cast<LibCore::Dxlib::BlendMode>(mode), param };
    }

    void SetBlendMode(const LibCore::Dxlib::BlendMode mode, const int param)
    {
        SetDrawBlendMode(static_cast<int>(mode), std::clamp(param, 0, 255));
    }

    void SetBlendModeAlpha(const LibCore::Dxlib::BlendMode mode, const float alpha01)
    {
        SetBlendMode(mode, static_cast<int>(std::clamp(alpha01, 0.0f, 1.0f) * 255.0f));
    }

    Color32 GetBright()
    {
        int r = 255, g = 255, b = 255;
        GetDrawBright(&r, &g, &b);
        return Color32(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    }

    void SetBright(const Color32& color)
    {
        SetDrawBright(color.R(), color.G(), color.B());
    }

    void SetBright(const int r, const int g, const int b)
    {
        SetDrawBright(r, g, b);
    }

    FilterMode GetFilterMode()
    {
        return static_cast<FilterMode>(GetDrawMode());
    }

    void SetFilterMode(const FilterMode mode)
    {
        SetDrawMode(static_cast<int>(mode));
    }

    glm::ivec2 ScreenSize()
    {
        int width = 0, height = 0;
        GetDrawScreenSize(&width, &height);
        return { width, height };
    }

    std::optional<glm::ivec2> GraphSize(const int graphHandle)
    {
        int width = 0, height = 0;
        if (GetGraphSize(graphHandle, &width, &height) != 0)
            return std::nullopt;
        return glm::ivec2(width, height);
    }

    void SetDrawArea(const int x1, const int y1, const int x2, const int y2)
    {
        DxLib::SetDrawArea(x1, y1, x2, y2);
    }

    void ResetDrawArea()
    {
        const glm::ivec2 size = ScreenSize();
        DxLib::SetDrawArea(0, 0, size.x, size.y);
    }

    ScopedDrawState::ScopedDrawState(const bool resetDrawAreaOnExit)
        : filter_(GetFilterMode()), blend_(GetBlendMode()), bright_(GetBright()), resetDrawAreaOnExit_(resetDrawAreaOnExit)
    {
    }

    ScopedDrawState::~ScopedDrawState()
    {
        if (resetDrawAreaOnExit_)
            ResetDrawArea();
        SetFilterMode(filter_);
        SetBlendMode(blend_.mode, blend_.param);
        SetBright(bright_);
    }

    void DrawRotaGraph(const glm::vec2& center, const double scale, const double angle, const int graphHandle, const bool transparent)
    {
        DrawRotaGraphF(center.x, center.y, scale, angle, graphHandle, ToFlag(transparent));
    }

    void DrawRotaGraph2(const glm::vec2& position, const glm::vec2& pivot, const double scale, const double angle, const int graphHandle, const bool transparent)
    {
        DrawRotaGraph2F(position.x, position.y, pivot.x, pivot.y, scale, angle, graphHandle, ToFlag(transparent));
    }

    void DrawRectModiGraph(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4,
                           const int srcX, const int srcY, const int srcWidth, const int srcHeight, const int graphHandle, const bool transparent)
    {
        DrawRectModiGraphF(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, srcX, srcY, srcWidth, srcHeight, graphHandle, ToFlag(transparent));
    }

    void DrawBox(const int x1, const int y1, const int x2, const int y2, const Color32& color, const bool fill)
    {
        DxLib::DrawBox(x1, y1, x2, y2, static_cast<unsigned int>(color.ToDxColor()), ToFlag(fill));
    }

    void DrawQuadrangleAA(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4, const Color32& color, const bool fill)
    {
        DxLib::DrawQuadrangleAA(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, static_cast<unsigned int>(color.ToDxColor()), ToFlag(fill));
    }

    void DrawCircleGauge(const glm::vec2& center, const double percent, const int graphHandle, const double startPercent, const double scale,
                         const bool reverseX, const bool reverseY)
    {
        DrawCircleGaugeF(center.x, center.y, percent, graphHandle, startPercent, scale, ToFlag(reverseX), ToFlag(reverseY));
    }

    void DrawString(const glm::vec2& position, const glm::vec2& scale, const std::string& utf8, const Color32& color, const int fontHandle,
                    const Color32& edgeColor)
    {
        const std::string sjis = LibCore::Dxlib::Utf8ToShiftJis(utf8);
        DrawExtendStringFToHandle(position.x, position.y, scale.x, scale.y, sjis.c_str(),
                                  static_cast<unsigned int>(color.ToDxColor()), fontHandle, static_cast<unsigned int>(edgeColor.ToDxColor()));
    }

    int StringWidth(const double scale, const std::string& utf8, const int fontHandle)
    {
        const std::string sjis = LibCore::Dxlib::Utf8ToShiftJis(utf8);
        return GetDrawExtendStringWidthToHandle(scale, sjis.c_str(), static_cast<int>(sjis.size()), fontHandle);
    }

    int FontSize(const int fontHandle)
    {
        return GetFontSizeToHandle(fontHandle);
    }
}
