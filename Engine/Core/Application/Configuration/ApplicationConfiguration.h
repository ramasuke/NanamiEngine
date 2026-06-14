#pragma once

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

        [[nodiscard]] static int GetWindowWidth();
        [[nodiscard]] static int GetWindowHeight();
        [[nodiscard]] static int GetWindowColorScale();

        static void SetWindowWidth(int width);
        static void SetWindowHeight(int height);
        static void SetWindowColorScale(int scale);

    private:
        static int windowWidth_;
        static int windowHeight_;
        static int windowColorScale_;
    };
}
