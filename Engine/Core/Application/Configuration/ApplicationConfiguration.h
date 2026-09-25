#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "../Display/WindowDisplayMode.h"

namespace NanamiEngine::Core::Application::Configuration
{
    enum class ApplicationMode
    {
        Editor,
        Game
    };

#if defined(NANAMI_GAME_BUILD)
    constexpr auto APPLICATION_MODE = ApplicationMode::Game;
#else
    constexpr auto APPLICATION_MODE = ApplicationMode::Editor;
#endif
    
    class NANAMI_API AppConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static int   GetWindowWidth();
        [[nodiscard]] static int   GetWindowHeight();
        [[nodiscard]] static int   GetWindowColorScale();
        static void                SetWindowWidth(int width);
        static void                SetWindowHeight(int height);
        static void                SetWindowColorScale(int scale);

        /** ゲーム起動時の表示モード (プレイヤーが選んだものが LocalPrefs にあればそちらが優先) */
        [[nodiscard]] static Display::WindowDisplayMode GetDefaultWindowMode();
        static void                                     SetDefaultWindowMode(Display::WindowDisplayMode mode);

        [[nodiscard]] static int   GetZBufferBitDepth();
        static void                SetZBufferBitDepth(int bitDepth);

        [[nodiscard]] static bool  GetAlwaysRun();
        static void                SetAlwaysRun(bool alwaysRun);

        [[nodiscard]] static int   GetShadowMapWidth();
        [[nodiscard]] static int   GetShadowMapHeight();
        static void                SetShadowMapWidth(int w);
        static void                SetShadowMapHeight(int h);
        [[nodiscard]] static float GetShadowAreaHalfSize();
        static void                SetShadowAreaHalfSize(float halfSize);

        [[nodiscard]] static float GetLightDirX();
        [[nodiscard]] static float GetLightDirY();
        [[nodiscard]] static float GetLightDirZ();
        static void                SetLightDirX(float x);
        static void                SetLightDirY(float y);
        static void                SetLightDirZ(float z);

        [[nodiscard]] static float GetLightDifR();
        [[nodiscard]] static float GetLightDifG();
        [[nodiscard]] static float GetLightDifB();
        static void                SetLightDifR(float r);
        static void                SetLightDifG(float g);
        static void                SetLightDifB(float b);

        [[nodiscard]] static int               GetParticleMax();
        static void                            SetParticleMax(int max);

        [[nodiscard]] static const std::string& GetAssetsDirectoryPath();
        static void                             SetAssetsDirectoryPath(const std::string& path);

        static void DrawConfigGUI();

    private:
        static int windowWidth_;
        static int windowHeight_;
        static int windowColorScale_;
        static Display::WindowDisplayMode defaultWindowMode_;

        static int zBufferBitDepth_;

        static bool alwaysRun_;

        static int shadowMapWidth_;
        static int shadowMapHeight_;
        static float shadowAreaHalfSize_;

        static float lightDirX_;
        static float lightDirY_;
        static float lightDirZ_;

        static float lightDifR_;
        static float lightDifG_;
        static float lightDifB_;

        static int         particleMax_;
        static std::string assetsDirectoryPath_;
    };
}
