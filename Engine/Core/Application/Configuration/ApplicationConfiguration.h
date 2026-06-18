#pragma once
#include <string>

namespace NanamiEngine::Core::Application::Configuration
{
    enum class ApplicationMode
    {
        Editor,
        Game
    };

    constexpr auto APPLICATION_MODE = ApplicationMode::Editor;
    
    class AppConfiguration final
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

        [[nodiscard]] static int   GetFixedUpdateRate();
        static void                SetFixedUpdateRate(int rate);

        [[nodiscard]] static int   GetShadowMapWidth();
        [[nodiscard]] static int   GetShadowMapHeight();
        static void                SetShadowMapWidth(int w);
        static void                SetShadowMapHeight(int h);

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

        [[nodiscard]] static float GetMaxDeltaTime();
        static void                SetMaxDeltaTime(float t);
        [[nodiscard]] static int   GetMaxPhysicsStep();
        static void                SetMaxPhysicsStep(int step);

        [[nodiscard]] static int               GetParticleMax();
        static void                            SetParticleMax(int max);

        [[nodiscard]] static const std::string& GetAssetsDirectoryPath();
        static void                             SetAssetsDirectoryPath(const std::string& path);

        static void DrawConfigGUI();

    private:
        static int windowWidth_;
        static int windowHeight_;
        static int windowColorScale_;

        static int fixedUpdateRate_;

        static int shadowMapWidth_;
        static int shadowMapHeight_;

        static float lightDirX_;
        static float lightDirY_;
        static float lightDirZ_;

        static float lightDifR_;
        static float lightDifG_;
        static float lightDifB_;

        static float       maxDeltaTime_;
        static int         maxPhysicsStep_;
        static int         particleMax_;
        static std::string assetsDirectoryPath_;
    };
}
