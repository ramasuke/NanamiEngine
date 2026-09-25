#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>
#include <string>

#include "vec2.hpp"
#include "../../../Module/Color/Color32.h"
#include "../../../../Libs/LibCore/DxLib/BlendMode.h"

// 2D 描画の DxLib を出さない入口 (docs/HotReload.md §2)。座標はピクセル、色は Color32、画像・フォントは int ハンドル
// (SpriteFile::GetDxLibHandle / TtfFontFile::DxLibHandle)。文字列は UTF-8 で受け取り中で Shift-JIS に変換する
namespace NanamiEngine::Platform::Draw2D
{
    /** 画像の拡大縮小フィルタ (DxLib の DX_DRAWMODE_*) */
    enum class FilterMode : int
    {
        Nearest     = 0,
        Bilinear    = 1,
        Anisotropic = 2,
    };

    struct NANAMI_API BlendState
    {
        LibCore::Dxlib::BlendMode mode  = LibCore::Dxlib::BlendMode::NoBlend;
        int                       param = 0; // 0..255
    };

    [[nodiscard]] NANAMI_API BlendState GetBlendMode();
    NANAMI_API void SetBlendMode(LibCore::Dxlib::BlendMode mode, int param);
    /** @brief alpha01 を 0..255 に直して SetBlendMode する */
    NANAMI_API void SetBlendModeAlpha(LibCore::Dxlib::BlendMode mode, float alpha01);

    [[nodiscard]] NANAMI_API Color32 GetBright();
    NANAMI_API void SetBright(const Color32& color);
    NANAMI_API void SetBright(int r, int g, int b);

    [[nodiscard]] NANAMI_API FilterMode GetFilterMode();
    NANAMI_API void SetFilterMode(FilterMode mode);

    /** @brief 描画先 (バックバッファ) の大きさ */
    [[nodiscard]] NANAMI_API glm::ivec2 ScreenSize();
    /** @brief 画像の大きさ。無効なハンドルなら nullopt */
    [[nodiscard]] NANAMI_API std::optional<glm::ivec2> GraphSize(int graphHandle);
    NANAMI_API void SetDrawArea(int x1, int y1, int x2, int y2);
    /** @brief 描画範囲を画面全体に戻す */
    NANAMI_API void ResetDrawArea();

    /** フィルタ・ブレンド・輝度を抜けるときに元へ戻す。resetDrawAreaOnExit で描画範囲も画面全体に戻す */
    class NANAMI_API ScopedDrawState final
    {
    public:
        explicit ScopedDrawState(bool resetDrawAreaOnExit = false);
        ~ScopedDrawState();
        ScopedDrawState(const ScopedDrawState&)            = delete;
        ScopedDrawState& operator=(const ScopedDrawState&) = delete;

    private:
        FilterMode filter_;
        BlendState blend_;
        Color32    bright_;
        bool       resetDrawAreaOnExit_;
    };

    NANAMI_API void DrawRotaGraph(const glm::vec2& center, double scale, double angle, int graphHandle, bool transparent = true);
    /** @param pivot 画像内の回転・拡大の中心 (ピクセル) */
    NANAMI_API void DrawRotaGraph2(const glm::vec2& position, const glm::vec2& pivot, double scale, double angle, int graphHandle, bool transparent = true);
    /** @brief 画像の src 矩形を 4 点 (左上・右上・右下・左下) に変形して描く */
    NANAMI_API void DrawRectModiGraph(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4,
                           int srcX, int srcY, int srcWidth, int srcHeight, int graphHandle, bool transparent = true);
    NANAMI_API void DrawBox(int x1, int y1, int x2, int y2, const Color32& color, bool fill);
    NANAMI_API void DrawQuadrangleAA(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4, const Color32& color, bool fill);
    /** @brief 円ゲージ。percent 0..100 */
    NANAMI_API void DrawCircleGauge(const glm::vec2& center, double percent, int graphHandle, double startPercent = 0.0, double scale = 1.0,
                         bool reverseX = false, bool reverseY = false);

    NANAMI_API void DrawString(const glm::vec2& position, const glm::vec2& scale, const std::string& utf8, const Color32& color, int fontHandle,
                    const Color32& edgeColor = Color32(0, 0, 0));
    /** @brief scale 倍で描いたときの文字列の幅 (ピクセル) */
    [[nodiscard]] NANAMI_API int StringWidth(double scale, const std::string& utf8, int fontHandle);
    /** @brief フォントを作ったときの大きさ (ピクセル) */
    [[nodiscard]] NANAMI_API int FontSize(int fontHandle);
}
