#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>
#include <string>

namespace NanamiEngine::Core::Application::Display
{
    /** メインウィンドウの表示方法。描画解像度 (AppConfiguration の WindowWidth/Height) はどのモードでも変わらない */
    enum class WindowDisplayMode
    {
        Windowed,
        Borderless,
        Fullscreen,
    };

    [[nodiscard]] NANAMI_API const char*       ToString(WindowDisplayMode mode);
    [[nodiscard]] NANAMI_API WindowDisplayMode WindowDisplayModeFromString(const std::string& text, WindowDisplayMode fallback);

    /** 表示モードの適用と切り替え。プレイヤーの選択は LocalPrefs/Display/WindowMode.json に残る */
    class NANAMI_API WindowDisplayModeController final
    {
    public:
        /** 起動時のモードを決めて DxLib_Init 前の設定を行う */
        static void ApplyBeforeInit();
        /** DxLib_Init 後にウィンドウのスタイル・サイズ・位置を合わせる */
        static void ApplyAfterInit();

        /** 次のフレーム終わりに切り替える。ImGui のフレーム途中でも呼べる */
        static void Request(WindowDisplayMode mode);
        /** Windowed と最後に使った全画面系 (Borderless / Fullscreen) を切り替える */
        static void RequestToggle();

        [[nodiscard]] static WindowDisplayMode Current();

        /** ScreenFlip の後に呼ぶ。Alt+Enter と保留中の切り替えを処理する */
        static void OnFrameEnd();

    private:
        static void Apply(WindowDisplayMode mode);
        static void Save();

        static WindowDisplayMode                current_;
        static WindowDisplayMode                lastFullscreen_;
        static std::optional<WindowDisplayMode> pending_;
        static bool                             toggleKeyHeld_;
    };
}
