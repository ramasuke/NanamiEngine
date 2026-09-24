#include "WindowDisplayMode.h"

#include <algorithm>
#include <exception>

#include <cereal/types/string.hpp>
#include "DxLib.h"

#include "../Configuration/ApplicationConfiguration.h"
#include "../../../Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Application::Display
{
    namespace
    {
        constexpr auto WINDOW_MODE_PREFS_PATH = "Display/";
        constexpr auto WINDOW_MODE_PREFS_KEY  = "WindowMode";

        /** SetWindowStyleMode の値 */
        constexpr int WINDOW_STYLE_DEFAULT    = 0;
        constexpr int WINDOW_STYLE_BORDERLESS = 2;

        struct ScreenRects
        {
            RECT monitor;
            RECT work;
        };

        ScreenRects MainWindowScreenRects()
        {
            // NOTE: DxLib_Init 前はウィンドウが無いのでプライマリモニターを使う
            const HWND     window  = GetMainWindowHandle();
            const HMONITOR monitor = window != nullptr
                ? MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY)
                : MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

            MONITORINFO info{};
            info.cbSize = sizeof(info);
            GetMonitorInfo(monitor, &info);
            return ScreenRects{ info.rcMonitor, info.rcWork };
        }

        void ApplyFullscreenSettings()
        {
            // 描画解像度はそのままモニターの解像度へ拡大する
            SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_DESKTOP);
            SetFullScreenScalingMode   (DX_FSSCALINGMODE_BILINEAR);
        }

        void ApplyBorderless()
        {
            const double screenWidth  = Configuration::AppConfiguration::GetWindowWidth();
            const double screenHeight = Configuration::AppConfiguration::GetWindowHeight();
            const RECT   monitor      = MainWindowScreenRects().monitor;

            SetWindowStyleMode           (WINDOW_STYLE_BORDERLESS);
            SetWindowSizeChangeEnableFlag(FALSE, TRUE);
            // NOTE: 16:9 以外のモニターでは引き伸ばす
            SetWindowSizeExtendRate((monitor.right - monitor.left) / screenWidth, (monitor.bottom - monitor.top) / screenHeight);
            SetWindowPosition(monitor.left, monitor.top);
        }

        void ApplyWindowed()
        {
            const int  screenWidth  = Configuration::AppConfiguration::GetWindowWidth();
            const int  screenHeight = Configuration::AppConfiguration::GetWindowHeight();
            const RECT work         = MainWindowScreenRects().work;

            RECT frame{ 0, 0, 0, 0 };
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
            const int frameWidth  = frame.right - frame.left;
            const int frameHeight = frame.bottom - frame.top;

            const int    workWidth  = work.right - work.left;
            const int    workHeight = work.bottom - work.top;
            const double fitRate    = (std::min)(static_cast<double>(workWidth  - frameWidth ) / screenWidth,
                                                 static_cast<double>(workHeight - frameHeight) / screenHeight);
            const double rate       = std::clamp(fitRate, 0.25, 1.0);

            SetWindowStyleMode           (WINDOW_STYLE_DEFAULT);
            SetWindowSizeChangeEnableFlag(TRUE, TRUE);
            SetWindowSizeExtendRate      (rate);

            const int windowWidth  = static_cast<int>(screenWidth  * rate) + frameWidth;
            const int windowHeight = static_cast<int>(screenHeight * rate) + frameHeight;
            SetWindowPosition(work.left + (std::max)(0, (workWidth  - windowWidth ) / 2),
                              work.top  + (std::max)(0, (workHeight - windowHeight) / 2));
        }
    }

    WindowDisplayMode                WindowDisplayModeController::current_        = WindowDisplayMode::Windowed;
    WindowDisplayMode                WindowDisplayModeController::lastFullscreen_ = WindowDisplayMode::Borderless;
    std::optional<WindowDisplayMode> WindowDisplayModeController::pending_        = std::nullopt;
    bool                             WindowDisplayModeController::toggleKeyHeld_  = false;

    const char* ToString(const WindowDisplayMode mode)
    {
        switch (mode)
        {
        case WindowDisplayMode::Windowed:   return "Windowed";
        case WindowDisplayMode::Borderless: return "Borderless";
        case WindowDisplayMode::Fullscreen: return "Fullscreen";
        }
        return "Windowed";
    }

    WindowDisplayMode WindowDisplayModeFromString(const std::string& text, const WindowDisplayMode fallback)
    {
        for (const auto mode : { WindowDisplayMode::Windowed, WindowDisplayMode::Borderless, WindowDisplayMode::Fullscreen })
        {
            if (text == ToString(mode))
                return mode;
        }
        return fallback;
    }

    void WindowDisplayModeController::ApplyBeforeInit()
    {
        // NOTE: エディタは ProjectConfig の既定値 (ゲーム用) を使わない
        const WindowDisplayMode defaultMode = Configuration::APPLICATION_MODE == Configuration::ApplicationMode::Game
            ? Configuration::AppConfiguration::GetDefaultWindowMode()
            : WindowDisplayMode::Windowed;
        const auto saved = Module::LocalPrefs::LoadOrDefaultWithPath<std::string>(WINDOW_MODE_PREFS_PATH, WINDOW_MODE_PREFS_KEY, std::string());
        current_ = WindowDisplayModeFromString(saved, defaultMode);
        if (current_ != WindowDisplayMode::Windowed)
            lastFullscreen_ = current_;

        ApplyFullscreenSettings();
        ChangeWindowMode(current_ == WindowDisplayMode::Fullscreen ? FALSE : TRUE);
    }

    void WindowDisplayModeController::ApplyAfterInit()
    {
        Apply(current_);
    }

    void WindowDisplayModeController::Request(const WindowDisplayMode mode)
    {
        pending_ = mode;
    }

    void WindowDisplayModeController::RequestToggle()
    {
        const WindowDisplayMode base = pending_.value_or(current_);
        Request(base == WindowDisplayMode::Windowed ? lastFullscreen_ : WindowDisplayMode::Windowed);
    }

    WindowDisplayMode WindowDisplayModeController::Current()
    {
        return current_;
    }

    void WindowDisplayModeController::OnFrameEnd()
    {
        const bool altDown   = CheckHitKey(KEY_INPUT_LALT) != 0 || CheckHitKey(KEY_INPUT_RALT) != 0;
        const bool toggleKey = altDown && CheckHitKey(KEY_INPUT_RETURN) != 0 && GetWindowActiveFlag() != 0;
        if (toggleKey && !toggleKeyHeld_)
            RequestToggle();
        toggleKeyHeld_ = toggleKey;

        if (!pending_)
            return;

        const WindowDisplayMode mode = *pending_;
        pending_.reset();
        if (mode == current_)
            return;

        Apply(mode);
        Save();
    }

    void WindowDisplayModeController::Apply(const WindowDisplayMode mode)
    {
        current_ = mode;
        if (mode != WindowDisplayMode::Windowed)
            lastFullscreen_ = mode;

        if (mode == WindowDisplayMode::Fullscreen)
        {
            ApplyFullscreenSettings();
            if (GetWindowModeFlag() != 0)
                ChangeWindowMode(FALSE);
            return;
        }

        if (GetWindowModeFlag() == 0)
            ChangeWindowMode(TRUE);

        if (mode == WindowDisplayMode::Borderless)
            ApplyBorderless();
        else
            ApplyWindowed();
    }

    void WindowDisplayModeController::Save()
    {
        try
        {
            Module::LocalPrefs::SaveWithPath<std::string>(WINDOW_MODE_PREFS_PATH, WINDOW_MODE_PREFS_KEY, ToString(current_));
        }
        catch (const std::exception& exception)
        {
            Module::LogWarning(std::string("WindowDisplayMode: failed to save: ") + exception.what());
        }
    }
}
