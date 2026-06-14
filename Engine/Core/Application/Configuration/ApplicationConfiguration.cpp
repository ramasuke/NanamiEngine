#include "ApplicationConfiguration.h"
#include "../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_WINDOW_WIDTH_SIZE  = 1920;
    constexpr auto DEFAULT_WINDOW_HEIGHT_SIZE = 1080;
    constexpr auto DEFAULT_WINDOW_COLOR_SCALE = 16;
    
    int AppConfiguration::windowWidth_      = DEFAULT_WINDOW_WIDTH_SIZE;
    int AppConfiguration::windowHeight_     = DEFAULT_WINDOW_HEIGHT_SIZE;
    int AppConfiguration::windowColorScale_ = DEFAULT_WINDOW_COLOR_SCALE;

    constexpr auto APP_CONFIG_PATH       = "Application/";
    constexpr auto APP_CONFIG_WIDTH_KEY  = "WindowWidth";
    constexpr auto APP_CONFIG_HEIGHT_KEY = "WindowHeight";
    constexpr auto APP_CONFIG_SCALE_KEY  = "WindowColorScale";

    void AppConfiguration::Load()
    {
        windowWidth_      = Module::ProjectConfig::LoadOrDefaultWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_WIDTH_KEY,  DEFAULT_WINDOW_WIDTH_SIZE);
        windowHeight_     = Module::ProjectConfig::LoadOrDefaultWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_HEIGHT_KEY, DEFAULT_WINDOW_HEIGHT_SIZE);
        windowColorScale_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_SCALE_KEY,  DEFAULT_WINDOW_COLOR_SCALE);
    }

    void AppConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_WIDTH_KEY,  windowWidth_);
        Module::ProjectConfig::SaveWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_HEIGHT_KEY, windowHeight_);
        Module::ProjectConfig::SaveWithPath<int>(APP_CONFIG_PATH, APP_CONFIG_SCALE_KEY,  windowColorScale_);
    }

    int  AppConfiguration::GetWindowWidth()           { return windowWidth_; }
    int  AppConfiguration::GetWindowHeight()          { return windowHeight_; }
    int  AppConfiguration::GetWindowColorScale()      { return windowColorScale_; }
    void AppConfiguration::SetWindowWidth(int w)      { windowWidth_      = w; }
    void AppConfiguration::SetWindowHeight(int h)     { windowHeight_     = h; }
    void AppConfiguration::SetWindowColorScale(int s) { windowColorScale_ = s; }
}
